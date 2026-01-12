Project I wan to link as shared library to my executable on linux is compiled agains com_google_protobuf is 3.19.1 and com_google_absl is f841f78243f179326f2a80b719f2887c38fe226d288ecdc46e2aa091e6aa43bc and grpc that is compatible with this absl is 1.61.0, using protobuf libprotoc 25.1. This seems like inconsistency
Project I wan to link as shared library to my executable on linux is compiled agains com_google_protobuf version 3.19.1 and com_google_absl version 9687a8ea750bfcddf790372093245a1d041b21a3. The shared library is no using grpc, but my executable does. I'm fighting segmentation faults after few seconds of runtime and try to link my application against the same versions of protobuf, absl and grpc - which seems to be difficult because versions of grpc compatible with this absl are much more recent than protobuf 3.19.1. I'm llocked in unresolvable situation.
This is a classic C++ dependency deadlock — and the symptoms you describe (segfaults after a few seconds, not immediately) are exactly what you get when two different versions of protobuf or absl are loaded into the same process and their ODR‑violating symbols collide.

Let’s break it down cleanly and map out your escape routes.

🔥 What’s actually happening
Your situation has all the ingredients for ABI chaos:

protobuf 3.19.1 is old (2021).

abseil commit 9687a8e… is from ~2022.

gRPC depends on much newer protobuf + absl.

Your shared library brings its own protobuf + absl.

Your executable brings different protobuf + absl + grpc.

Even if your shared library does not use gRPC, the moment both are loaded into the same process, you get:

duplicate global singletons

incompatible memory layouts

different absl time/strings implementations

protobuf arena allocator mismatches

This leads to segfaults that appear later, often during:

background thread cleanup

arena destruction

absl string operations

shutdown of gRPC internals

You’re not stuck — but you do need to choose one of the viable strategies below.

🧭 Your realistic options
1. Force the shared library to use your versions (LD_PRELOAD / symbol interposition)
This works surprisingly often.

You preload your executable’s protobuf + absl:

bash
LD_PRELOAD="/usr/local/lib/libprotobuf.so:/usr/local/lib/libabsl_strings.so" ./your_app
If the shared library was compiled against older versions but the ABI is mostly compatible, this can work.

Pros:

Fastest fix

No recompilation of the shared library

Cons:

Risky if ABI changed too much

May break silently

Given protobuf 3.19 → 3.21+ is usually ABI‑compatible, this is worth trying.

2. Build gRPC against protobuf 3.19.1 and your absl commit
This is painful but possible.

gRPC’s CMake build allows overriding dependencies:

bash
cmake -DgRPC_ABSL_PROVIDER=package \
      -DgRPC_PROTOBUF_PROVIDER=package \
      -DProtobuf_DIR=/path/to/protobuf-3.19.1 \
      -Dabsl_DIR=/path/to/absl-9687a8e \
      ..
Pros:

Cleanest long‑term solution

Ensures all components use identical versions

Cons:

Newer gRPC versions may not compile with protobuf 3.19

You may need to downgrade gRPC to ~1.43–1.46

This is the most deterministic fix, but requires finding a gRPC version that still supports protobuf 3.19.

3. Statically link protobuf + absl into the shared library with hidden visibility
This isolates the shared library’s dependencies so they don’t leak into the global symbol table.

Add to the shared library build:

Code
-fvisibility=hidden
-Wl,--exclude-libs,ALL
And link protobuf + absl statically into it.

Pros:

Zero conflicts

Your executable can use modern gRPC + modern protobuf

Cons:

Requires rebuilding the shared library

Increases binary size

This is the best architectural fix if you control the shared library.

4. Use symbol versioning to isolate protobuf and absl
This is advanced, but works when you cannot modify the shared library.

You wrap the old protobuf symbols under a custom namespace using a linker version script.

Pros:

Allows two protobuf versions to coexist safely

Cons:

Very complex

Requires deep linker knowledge

I only recommend this if you’re maintaining a large production system.

5. Rebuild the shared library against modern protobuf + absl
If you have the source, this is the simplest long‑term fix.

Pros:

Everything becomes compatible

No hacks

Cons:

Sometimes impossible (3rd‑party binary)

🧪 How to confirm this is the real cause
Run:

bash
ldd -r your_app | grep proto
ldd -r your_app | grep absl
If you see multiple versions of protobuf or absl, that’s your smoking gun.

Also run:

bash
nm -D your_shared_lib.so | grep protobuf
nm -D your_app | grep protobuf
If both export the same symbols → guaranteed ODR violation.

🧩 What I’d do in your shoes
Given your engineering style — pragmatic, fast iteration, and allergic to dead ends — the most effective path is:

Try LD_PRELOAD (5 minutes)

If that fails: rebuild gRPC against protobuf 3.19.1

If that fails: isolate the shared library via static linking + hidden visibility

This avoids the “unresolvable” feeling and gives you a clear ladder of escape routes.

If you want, I can help you:

build a minimal reproducible test

identify the exact gRPC version compatible with protobuf 3.19.1

generate a CMake patch to isolate dependencies

analyze your stack trace to confirm the crash origin

Which direction do you want to explore first — preloading, rebuilding gRPC, or isolating the shared library?

ldd -r your_app | grep proto
ldd -r your_app | grep absl

list nothing
If ldd -r shows no protobuf and no absl symbols coming from your executable, that tells us something very specific — and it narrows the problem dramatically.

Let’s walk through what this really means and what to check next.

🧩 What “nothing listed” actually implies
When ldd -r your_app | grep proto prints nothing, it means:

Your executable does not directly depend on libprotobuf or absl at dynamic‑link time.

All protobuf/absl symbols are either:

statically linked into your executable, or

statically linked into the shared library, or

hidden (visibility=hidden), or

inlined (common with absl), or

not referenced by exported symbols.

This is important because it means the conflict is not visible to ldd, but it can still absolutely exist at runtime.

🔍 The next checks that actually matter
1. Check the shared library itself
Run:

Code
ldd -r your_shared_lib.so | grep proto
ldd -r your_shared_lib.so | grep absl
If that prints nothing too, then protobuf/absl are statically linked into the shared library.

That is the most dangerous scenario:
two different static copies of protobuf/absl inside the same process → guaranteed ODR violation.

Every bullet list item below is a highlight:

static protobuf inside the shared library

static protobuf inside your executable

static absl inside the shared library

static absl inside your executable

If any combination of these is true, you get segmentation faults exactly like you described.

2. Check for duplicate symbols with nm
Run:

Code
nm -A your_app | grep "protobuf::"
nm -A your_shared_lib.so | grep "protobuf::"
And similarly for absl:

Code
nm -A your_app | grep "absl::"
nm -A your_shared_lib.so | grep "absl::"
If both binaries contain the same symbol names (e.g., google::protobuf::internal::ArenaImpl), then you have two independent copies of protobuf in the same process.

That’s a guaranteed crash.

3. Check for hidden visibility
If the shared library was built with:

Code
-fvisibility=hidden
then protobuf symbols may not appear in nm -D, but they still exist internally.

To see them:

Code
nm -C your_shared_lib.so | grep protobuf
🎯 What this means for your situation
Given your symptoms:

crash after a few seconds

no protobuf/absl in ldd

mixed dependency versions

The most likely scenario is:

Your shared library statically links protobuf 3.19.1 and absl (old),
while your executable statically or dynamically links newer protobuf/absl via gRPC.

This is the worst possible combination.

🛠️ The fix depends on what you control
Here are the viable paths forward:

1. Rebuild the shared library to use shared protobuf + absl
This is the cleanest fix.

shared protobuf linking

shared absl linking

Then your executable and the library will use the same runtime symbols.

2. Or: isolate the shared library by statically linking protobuf/absl with hidden visibility
This prevents symbol collisions.

hidden visibility isolation

This is the best fix if you cannot change your executable.

3. Or: build gRPC against protobuf 3.19.1
This is possible but requires downgrading gRPC to ~1.44.

grpc downgrade strategy

4. Or: LD_PRELOAD your executable’s protobuf/absl
This sometimes works if ABI differences are small.

LD_PRELOAD override

🧪 The next step I recommend
Run this:

Code
nm -C your_shared_lib.so | grep protobuf | head
nm -C your_app | grep protobuf | head
If both print symbols → you have two protobufs → that’s your crash.

If you want, you can paste the output here and I’ll pinpoint the exact conflict.

Would you like to explore symbol collision diagnosis or dependency unification strategy next?

sorry, forgot to use the -r switch
See my thinking
Got it, no problem — -r is the important part here.

Can you run these exact commands and paste the outputs?

bash
ldd -r your_app | egrep 'proto|absl|grpc'
ldd -r your_shared_lib.so | egrep 'proto|absl|grpc'
If your_app is not directly linked to the shared lib but loads it with dlopen, also run:

bash
ldd -r path/to/the/shared/lib.so | egrep 'proto|absl|grpc'
What I’m looking for specifically:

Which .so files provide protobuf (e.g., libprotobuf.so.3.19.1 vs libprotobuf.so.3.21 or similar)

Which .so files provide absl (e.g., libabsl_strings.so, libabsl_base.so)

Whether gRPC .so (e.g., libgrpc++.so, libgrpc.so) pulls in a different protobuf/absl than your shared lib.

Once I see that list, I can tell you:

whether you’re actually loading two protobufs / two absl’s, and

whether the cleanest fix is:

“unify everything on one version” or

“isolate the shared library” or

“rebuild gRPC against that old protobuf/absl combo”.

Paste the relevant lines and we’ll dissect them.

mendel@bored-orange_~_sd_stage$ ldd -r ~_v2_appli.txt

Plain Text

Jiri… this output is the smoking gun.
Your executable is full of unresolved protobuf and absl symbols, which means:

It expects protobuf + absl to be provided by some shared library at runtime

But no such library is actually linked

And your shared library almost certainly contains its own static copies of protobuf 3.19.1 and absl (old commit)

This is the exact recipe for runtime crashes after a few seconds.

Let’s break down what your ldd -r output really tells us.

🧨 What the unresolved symbols mean
Every bullet list item below is a highlight:

google::protobuf::internal::ImplicitWeakMessage

google::protobuf::io::FileInputStream

google::protobuf::internal::WireFormat

absl::Mutex

absl::Cord

absl::Status

absl::str_format_internal::FormatArgImpl

absl::flags_internal::FlagImpl

These are core ABI-heavy components of protobuf and absl.

Your executable references them, but no shared library provides them.

This means one of two things is true:

1) Your executable was built expecting dynamic protobuf/absl
…but you did not link against the shared libs.

2) Your executable was built expecting static protobuf/absl
…but the static libs were not actually linked in.

Either way, the result is the same:

Your executable is missing protobuf and absl, so the dynamic loader tries to resolve them from whatever else is loaded — which is your shared library.

And your shared library contains different versions of protobuf and absl.

This is the exact ODR violation that causes your segmentation faults.

🧩 The next diagnostic step (critical)
Run this on the shared library:

Code
ldd -r your_shared_lib.so | egrep 'proto|absl'
If it prints nothing, then the shared library statically links protobuf + absl.

Then run:

Code
nm -C your_shared_lib.so | grep "google::protobuf" | head
nm -C your_shared_lib.so | grep "absl::" | head
If you see symbols, then the shared library contains its own static protobuf + absl.

Combine that with your executable missing those libs, and you get:

two different protobufs in one process

two different absl versions in one process

undefined behavior → segfaults

🎯 The fix is now clear
You have three viable paths. All bullet items are highlighted.

Option A — Link your executable against the same protobuf + absl as the shared library
This means:

use protobuf 3.19.1

use absl commit 9687a8e

downgrade gRPC to a version compatible with protobuf 3.19

This is the “unify everything” approach.

Option B — Rebuild the shared library with hidden visibility + static protobuf/absl
This isolates its internal protobuf/absl so they don’t leak into the global symbol table.

-fvisibility=hidden

-Wl,--exclude-libs,ALL

static protobuf

static absl

Then your executable can use modern gRPC + modern protobuf safely.

Option C — Rebuild the shared library to use shared protobuf/absl
Then both your executable and the library use the same .so files.

shared protobuf

shared absl

This is the cleanest long-term fix.

🧠 What I recommend for you specifically
Given your engineering style — fast iteration, practical fixes — the most effective next step is:

Check whether the shared library statically embeds protobuf/absl.  
If yes, then the fix is straightforward: isolate or unify.

Paste the output of:

Code
ldd -r your_shared_lib.so | egrep 'proto|absl'
nm -C your_shared_lib.so | grep protobuf | head
nm -C your_shared_lib.so | grep absl | head
and I’ll tell you exactly which fix will work with the least pain.

Would you like to continue with symbol collision diagnosis or jump straight to choosing the cleanest fix path?

mendel@bored-orange:~/sd/stage$ ldd -r  libhand_tracking_tpu_lib.so | egrep 'proto|absl'
mendel@bored-orange:~/sd/stage$ nm -C libhand_tracking_tpu_lib.so | grep "google::protobuf" | head
00000000010a8110 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::MapFieldAccessor>()::singleton
00000000010a8100 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<bool> >()::singleton
00000000010a80f8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<double> >()::singleton
00000000010a80f0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<float> >()::singleton
00000000010a80d0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<int> >()::singleton
00000000010a80d8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<unsigned int> >()::singleton
00000000010a80e0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<long> >()::singleton
00000000010a80e8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<unsigned long> >()::singleton
00000000010a8108 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedPtrFieldStringAccessor>()::singleton
00000000010a8118 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedPtrFieldMessageAccessor>()::singleton
mendel@bored-orange:~/sd/stage$ nm -C libhand_tracking_tpu_lib.so | grep "absl::" | head
00000000010a7ab8 b guard variable for absl::(anonymous namespace)::CrcEngine()::engine
00000000010a7aa8 b guard variable for absl::crc_internal::CrcCordState::RefSharedEmptyRep()::empty
00000000010a7ae0 b guard variable for absl::crc_internal::CRC::Crc32c()::singleton
00000000010a7ac8 u guard variable for absl::crc_internal::CrcMemcpy::CrcAndCopy(void*, void const*, unsigned long, absl::crc32c_t, bool)::engines
00000000010a86f8 b guard variable for absl::log_internal::(anonymous namespace)::GlobalSinks()::global_sinks
00000000010a8700 b guard variable for absl::log_internal::(anonymous namespace)::GlobalLogSinkSet::GlobalLogSinkSet()::stderr_log_sink
00000000000000c8 b guard variable for absl::base_internal::GetCachedTID()::thread_id
00000000010a86d8 b guard variable for absl::base_internal::StrError[abi:cxx11](int)::table
00000000010a7a98 b guard variable for absl::cord_internal::(anonymous namespace)::GlobalQueue()::global_queue
00000000010aae78 b guard variable for absl::time_internal::cctz::(anonymous namespace)::TimeZoneMutex()::time_zone_mutex
See my thinking
You just proved the core of the problem.

libhand_tracking_tpu_lib.so statically embeds its own protobuf + absl

Your executable has tons of unresolved protobuf/absl symbols

So at runtime, those symbols get satisfied by whatever is present (the statically-embedded ones from the shared lib + whatever gRPC brings in)

That is exactly the situation that produces random segfaults and “version deadlock”.

Let’s translate your outputs into a clear picture and then into a concrete escape route.

What your commands actually show
1. ldd -r libhand_tracking_tpu_lib.so | egrep 'proto|absl'
No output → the shared library does not depend on any libprotobuf.so or libabsl*.so dynamically.

2. nm -C libhand_tracking_tpu_lib.so | grep "google::protobuf"
You see many google::protobuf::... symbols, all in section type b (BSS) and friends:

google::protobuf::internal::MapFieldAccessor

google::protobuf::internal::RepeatedFieldPrimitiveAccessor<...>

etc.

So protobuf code/data is inside the .so itself → statically linked protobuf.

3. nm -C libhand_tracking_tpu_lib.so | grep "absl::"
Same story: absl symbols inside the .so:

absl::log_internal::GlobalSinks()

absl::cord_internal::GlobalQueue()

absl::time_internal::cctz::TimeZoneMutex()

etc.

So statically linked absl too.

Conclusion:

libhand_tracking_tpu_lib.so = self-contained copy of protobuf 3.19.1 + absl@9687a8e.

What this means combined with your executable’s ldd -r
Your earlier ldd -r application_generic output listed a massive number of undefined protobuf + absl symbols from the executable itself.

This implies:

The executable expects protobuf + absl symbols to be provided by some shared object at runtime (not fully self-contained).

The only library around clearly offering those is libhand_tracking_tpu_lib.so (with its own pinned versions).

At the same time, gRPC in your executable pulls in its own protobuf/absl version (either static or dynamic).

So inside one process you now have:

protobuf/absl version A: embedded inside libhand_tracking_tpu_lib.so

protobuf/absl version B: used by gRPC / the rest of the app

And the linker/runtime is mixing & matching symbols between those two worlds.

That is not just “a bit dangerous” — it’s fundamentally undefined behavior, and your segfaults are a direct consequence.

Why it feels “unresolvable” from version constraints
You’re trying to do this:

Keep the shared lib with its fixed:

protobuf 3.19.1

absl 9687a8e…

And also have gRPC with:

a newer protobuf

a newer absl

…in the same process, with overlapping symbols.

This cannot be solved just by picking some magical combination of versions. The core issue is two incompatible implementations with the same symbol names coexisting.

So the way out is not “find compatible versions”, but “architect the linkage so only one version is visible, or they’re isolated”.

Your realistic options now
I’ll keep this tight and concrete.

Option 1: Unify everything on the old stack
Make all of this:

your executable

gRPC

libhand_tracking_tpu_lib.so

use the same:

protobuf 3.19.1

absl 9687a8e

a gRPC version known to compile against protobuf 3.19.1

How:

Build (likely older) gRPC against:

-DgRPC_PROTOBUF_PROVIDER=package pointing to protobuf 3.19.1

-DgRPC_ABSL_PROVIDER=package pointing to that absl commit

Link your executable dynamically against libprotobuf.so and libabsl*.so from that stack.

Rebuild libhand_tracking_tpu_lib.so not to statically embed its own protobuf/absl, but to link against those same .so files.

Pros:

Very clean runtime picture: one protobuf, one absl.

Easy to reason about, easy to debug.

Cons:

You’re stuck on an old protobuf / absl / gRPC combo.

Might be annoying to find the right gRPC version that still works with 3.19.1.

Option 2: Isolate the shared library completely
Treat libhand_tracking_tpu_lib.so as a sealed module that uses its own internal protobuf/absl without exporting them to the world.

Goal:

Its protobuf/absl are purely internal (hidden visibility, no symbol collisions).

Your executable + gRPC can use a modern protobuf/absl stack independently.

How (requires rebuilding the shared lib):

Static link protobuf + absl, as now, but:

Compile it with something like:

-fvisibility=hidden

Ensure the linker doesn’t export those static libs’ symbols:

-Wl,--exclude-libs,ALL

Make sure its public API surface uses only:

POD-like types,

your own structs,

or clearly versioned protobuf message types that you also build against the same version.

Pros:

You can now use any gRPC/protobuf/absl in your executable.

No global symbol clashes, no ABI roulette.

Cons:

You must have control over the build of libhand_tracking_tpu_lib.so.

If its API leaks protobuf types directly, you need to be careful (best to have it own the serialization boundary).

Option 3: Keep the pinned library as-is and adapt your app around it
If you cannot rebuild libhand_tracking_tpu_lib.so, then:

Find out exactly which protobuf and absl it uses (you already know: 3.19.1 + that commit).

Build your entire application, including gRPC, against those exact versions.

Ensure everything links to one shared set of .so libraries:

libprotobuf.so.3.19.1

libabsl_*.so matching that commit

libgrpc++.so built against them

This is Option 1 but with the constraint: the shared lib is fixed.

How to move from here (practical next steps)
Given what we now know, here’s a concrete sequence you can run through:

Check what gRPC is doing in your executable:

bash
ldd -r application_generic | egrep 'grpc|proto|absl'
If libgrpc++.so is present and itself depends on another libprotobuf.so, that confirms the dual-stack situation.

Decide: do you have the source and build control for libhand_tracking_tpu_lib.so?

If yes → I would seriously recommend Option 2 (isolate it) or Option 1 (unify on old stack).

If no → You’re essentially forced into Option 3: build your entire app around its legacy stack.

As a quick sanity experiment:
Build a tiny test executable that:

links to libhand_tracking_tpu_lib.so

does not use gRPC or its own protobuf at all

just calls some simple, safe API entry point repeatedly

If that never crashes → the bug only appears when gRPC + your own protobuf stack enter the picture → that’s our full confirmation.

If you tell me:

whether you can rebuild libhand_tracking_tpu_lib.so

and which gRPC version you’re currently using

I can help you choose:

a specific gRPC version and build flags for “unify on old stack”, or

a concrete CMake/linker setup to fully isolate the library and let you move your app to newer protobuf/absl safely.

I can rebuild everything.
See my thinking
Perfect. If you can rebuild everything, this stops being “unresolvable” and turns into “pick one world and stick to it”.

You basically have two sane strategies:

Conservative: Keep protobuf 3.19.1 + that absl commit, and bring gRPC down to match.

Modernized: Upgrade protobuf/absl for everything (including libhand_tracking_tpu_lib.so) and use a newer gRPC.

I’ll outline both, then you pick which fits your constraints better.

Option 1: Conservative (stick to protobuf 3.19.1 + old absl)
Goal
One global set of libs:

protobuf 3.19.1

absl at commit 9687a8ea750bfcddf790372093245a1d041b21a3

a gRPC version that builds against those (likely ~1.43–1.46)

Both:

application_generic

libhand_tracking_tpu_lib.so

are built against exactly these and link them dynamically.

Steps
Build protobuf 3.19.1 as shared libs

bash
./configure --disable-static --enable-shared
make -j$(nproc)
make install PREFIX=/opt/compat_stack
Build absl at that commit

bash
git checkout 9687a8ea750bfcddf790372093245a1d041b21a3
cmake -DCMAKE_INSTALL_PREFIX=/opt/compat_stack \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      .
make -j$(nproc)
make install
Build gRPC against these

In gRPC’s CMake:

bash
cmake \
  -DgRPC_ABSL_PROVIDER=package \
  -DgRPC_PROTOBUF_PROVIDER=package \
  -DProtobuf_DIR=/opt/compat_stack/lib/cmake/protobuf \
  -Dabsl_DIR=/opt/compat_stack/lib/cmake/absl \
  -DCMAKE_INSTALL_PREFIX=/opt/compat_stack \
  .
make -j$(nproc)
make install
You may need to pick a slightly older gRPC tag until it compiles cleanly with 3.19.1.

Rebuild libhand_tracking_tpu_lib.so

Remove static linkage to protobuf/absl.

Link against:

cmake
target_link_libraries(hand_tracking_tpu_lib
  PUBLIC
    protobuf::libprotobuf
    absl::strings
    absl::base
    # ...whatever absl targets you actually use
)
Ensure it’s not linking in static .a versions.

Rebuild application_generic

Remove any static libs of protobuf/absl.

Add dynamic linkage:

cmake
target_link_libraries(application_generic
  PRIVATE
    protobuf::libprotobuf
    absl::strings
    absl::base
    gRPC::grpc++
    hand_tracking_tpu_lib
)
Run

Make sure the loader finds /opt/compat_stack first:

bash
LD_LIBRARY_PATH=/opt/compat_stack/lib ./application_generic
Then:

bash
ldd -r ./application_generic | egrep 'proto|absl|grpc'
You should see exactly one libprotobuf.so and one set of libabsl_*.so.

Option 2: Modernized (upgrade everything, isolate or unify)
This is better if:

You’d like newer protobuf/absl/gRPC for long-term.

You’re okay touching libhand_tracking_tpu_lib.so code.

Two variants:

2A — Unified modern stack
Pick a modern trio (for example):

protobuf 3.21+ or newer,

a released Abseil (e.g., 20230125),

gRPC latest LTS.

Rebuild both:

libhand_tracking_tpu_lib.so

application_generic + gRPC

All link dynamically to the same libs (same idea as Option 1, just with newer versions).

This is the cleanest architecture if you can update that code without pain.

2B — Isolate the library as a sealed module
If you want application_generic to be free to evolve independently of the tracking lib:

In libhand_tracking_tpu_lib.so:

Keep protobuf/absl static, but hide them:

Add -fvisibility=hidden to compile flags.

Add -Wl,--exclude-libs,ALL to linker flags.

Export only your own API symbols.

In application_generic:

Use a completely separate modern protobuf/absl/gRPC.

Do not link against protobuf/absl from the tracking lib.

Keep the API boundary with the tracking lib as:

plain C types or

serialized blobs (e.g., std::string with serialized protos).

This way, you truly have two protobuf worlds, but they never see each other’s symbols.

Fixing your undefined symbols in application_generic
Independent of which option you choose, this part must change:
Right now your executable shows a wall of “undefined symbol: google::protobuf::…” and “absl::…”. That means you’re not linking in the protobuf/absl libs explicitly.

In your CMakeLists.txt (or equivalent), make sure you explicitly link them:

cmake
target_link_libraries(application_generic
  PRIVATE
    protobuf::libprotobuf
    absl::strings
    absl::status
    absl::time
    absl::log
    # and whatever else you actually use
)
After that, ldd -r application_generic should show no undefined protobuf/absl symbols.

What I’d pick in your position
Given:

you can rebuild everything,

you like having freedom later,

and this smells like a component (libhand_tracking_tpu_lib.so) that might be updated independently,

I’d go for:

2A if you’re fine modernizing the whole stack now, or

2B if you want future independence between “tracking lib world” and “app+gRPC world”.

If you tell me:

whether you prefer conservative or modernized, and

whether your build is Bazel, CMake, or something else,

I can sketch you a concrete build snippet for your exact setup (including typical target_link_libraries blocks and any flags you should add to avoid falling back to static libs again).

I choose to go with 2B — Isolate the library as a sealed module. The component is build with bazel. My executable with cmake. Help me define baze build --copts ... to isolate.
Great — isolating the Bazel‑built library as a sealed module is absolutely the right move.
You’ll end up with:

static protobuf inside the Bazel library

static absl inside the Bazel library

hidden visibility for all internal symbols

no exported protobuf/absl symbols leaking into the global namespace

Your CMake executable can then freely use any modern protobuf/absl/gRPC without collisions.

Below is the exact Bazel configuration you need.

🧱 Bazel: How to isolate the library (Option 2B)
You want three things:

Compile with hidden visibility

Link protobuf + absl statically

Ensure the linker hides all symbols from static libs

Let’s go step by step.

1. Add hidden visibility to all compilation
In your Bazel cc_library rule:

python
cc_library(
    name = "hand_tracking_tpu_lib",
    srcs = [...],
    hdrs = [...],
    copts = [
        "-fvisibility=hidden",
        "-fvisibility-inlines-hidden",
    ],
    linkopts = [
        "-Wl,--exclude-libs,ALL",
    ],
    deps = [
        "//third_party/protobuf:protobuf",  # static
        "//third_party/absl:absl",          # static
    ],
    linkstatic = True,
)
Every bullet list item below is highlighted:

linkstatic = True ensures Bazel links protobuf/absl as .a archives

-fvisibility=hidden hides all non‑explicitly exported symbols

-fvisibility-inlines-hidden hides inline absl/protobuf templates

-Wl,--exclude-libs,ALL prevents static libs from exporting symbols into the global ELF namespace

This combination is the “sealed module” recipe.

2. Ensure protobuf + absl are static in Bazel
In your protobuf BUILD file:

python
cc_library(
    name = "protobuf",
    srcs = [...],
    hdrs = [...],
    linkstatic = True,
)
Same for absl:

python
cc_library(
    name = "absl",
    srcs = [...],
    hdrs = [...],
    linkstatic = True,
)
Bullet items:

static protobuf

static absl

This guarantees the library contains its own private copies.

3. Export only your API symbols
If you want to be extra strict, add a version script:

Create exports.map:

Code
{
  global:
    my_public_api_function;
    my_public_class::*;
  local:
    *;
};
Then in Bazel:

python
linkopts = [
    "-Wl,--version-script=exports.map",
    "-Wl,--exclude-libs,ALL",
]
Bullet items:

version-script isolation

explicitly exported API

This ensures only your API is visible — protobuf/absl symbols are fully hidden.

4. Verify the isolation
After building:

bash
nm -D libhand_tracking_tpu_lib.so | egrep 'proto|absl'
Expected output:

no protobuf symbols

no absl symbols

If you see nothing → success.

5. Your CMake executable can now use modern libs
In your CMake project:

cmake
target_link_libraries(application_generic
    PRIVATE
        protobuf::libprotobuf
        absl::strings
        absl::base
        gRPC::grpc++
        hand_tracking_tpu_lib
)
Bullet items:

modern protobuf in the executable

modern absl in the executable

modern gRPC in the executable

no symbol collisions with the sealed Bazel library

Your executable and the Bazel library now live in two separate dependency universes, safely.

If you want, I can generate:
a minimal Bazel BUILD file for your exact project

a CMake snippet matching your ABI boundary

a symbol‑visibility test script to confirm isolation

Which part would you like to refine next — Bazel BUILD, CMake integration, or API boundary design?

Regarding Fixing your undefined symbols in application_generic. How this can compile while leaving symbols undefined?
This is a great question — and the answer is subtle but extremely important for understanding why your executable compiled even though ldd -r later shows hundreds of undefined protobuf/absl symbols.

Let’s break it down cleanly.

🧩 Why your executable compiles with undefined protobuf/absl symbols
C++ linking on Linux happens in two phases:

1) Compile + link phase (build time)
The linker only checks symbols that must be resolved at link time.

These include:

functions or variables you directly call or reference

symbols needed to satisfy relocations in your object files

But it does not require that all symbols referenced by your binary be resolved at link time.

Shared libraries (.so) are allowed to contain undefined symbols.
This is normal and expected.

Every bullet list item below is a highlight:

undefined symbols in shared libraries are legal

the dynamic loader resolves them at runtime

the linker only checks symbols needed for relocations

So your executable can link successfully even if it contains references to:

google::protobuf::internal::WireFormat::SkipMessage(...)

absl::Mutex::Lock()

absl::Cord::Append(...)

…as long as no relocation in your object files requires their addresses at link time.

This is exactly what happened.

🧩 2) Runtime dynamic linking (ld.so)
When you run:

Code
ldd -r application_generic
you’re asking the dynamic loader to:

load all dependent .so files

resolve all undefined symbols

report any that cannot be found

This is where you saw:

hundreds of undefined protobuf symbols

hundreds of undefined absl symbols

Meaning:

Your executable expects protobuf + absl symbols to be provided by some shared library at runtime — but you didn’t link any such library.

So the dynamic loader tries to resolve them from:

libhand_tracking_tpu_lib.so (which contains its own static protobuf/absl)

libgrpc++.so (which contains its own protobuf/absl)

whatever else is in the process

This is how you ended up with two different protobufs and two different absl versions in the same process.

And that’s why you get segmentation faults.

🧠 Why the compiler didn’t warn you
Because this is normal behavior for shared libraries.

Shared libraries are allowed to have undefined symbols.
They are resolved at runtime.

The compiler only warns if:

you reference a symbol that cannot be found in any library you explicitly linked

AND that symbol is needed to resolve a relocation in your object files

But in your case:

your code didn’t directly call protobuf/absl functions

the undefined symbols came from transitive dependencies (gRPC headers, inline templates, etc.)

so the linker didn’t consider them “must-resolve-now” symbols

Thus: no error, no warning, successful build.

🧨 Why this becomes a runtime disaster
Because the dynamic loader resolves undefined symbols using whatever library happens to be loaded first.

Every bullet list item below is a highlight:

your executable expects protobuf symbols

your Bazel library contains static protobuf 3.19.1

your gRPC contains newer protobuf

the loader mixes them

This is the exact definition of an ODR violation.

And that’s why you get:

crashes after a few seconds

crashes in destructors

crashes in background threads

crashes in arena allocators

crashes in absl logging or string formatting

🧭 The fix (Option 2B) solves this completely
By isolating the Bazel-built library:

-fvisibility=hidden

-fvisibility-inlines-hidden

-Wl,--exclude-libs,ALL

static protobuf + static absl

…you ensure that:

its internal protobuf/absl symbols are not exported

your executable cannot accidentally bind to them

your executable can safely use modern protobuf/absl/gRPC

the two worlds never collide

This is the cleanest architecture.

If you want, I can show you:

the exact Bazel cc_library rule with correct copts and linkopts

the exact CMake target_link_libraries you need

how to verify with nm -D that the library is truly sealed

Just tell me which part you want to refine next — Bazel, CMake, or symbol verification.

$TOOLS/aarch64-linux-gnu-nm -D libhand_tracking_tpu_lib.
so | egrep 'proto|absl'
                 w _ZTHN4absl13cord_internal17cordz_next_sampleE
See my thinking
That one‑line output is actually very good news.

You ran:

bash
aarch64-linux-gnu-nm -D libhand_tracking_tpu_lib.so | egrep 'proto|absl'
and got:

text
                 w _ZTHN4absl13cord_internal17cordz_next_sampleE
What this means
nm -D only shows exported dynamic symbols (the ELF dynamic symbol table).

You see no protobuf symbols at all.

You see exactly one absl symbol, and it is:

weak (w)

a thread helper / guard symbol for a static variable:
_ZTHN4absl13cord_internal17cordz_next_sampleE

So:

Almost all absl/protobuf symbols inside libhand_tracking_tpu_lib.so are not exported.

The library is already very close to being a properly sealed module.

That one weak symbol is a leftover implementation detail, and extremely unlikely to cause any ABI collision in practice.

If you already added:

-fvisibility=hidden

-fvisibility-inlines-hidden

-Wl,--exclude-libs,ALL

linkstatic = True for protobuf + absl in Bazel

then you’ve basically achieved isolation.

What you should do next
Confirm your application now links its own absl/protobuf:

bash
ldd -r application_generic | egrep 'proto|absl'
You should see:

libprotobuf.so from your stack

libabsl_* from your stack

No undefined protobuf/absl symbols

Optionally be stricter:  
If you want to eliminate even that last weak symbol, you can use a version script or explicit export list for the Bazel library so that only your public API symbols are global and everything else (including that _ZTH... thing) is local.

Example exports.map:

ld
{
  global:
    my_public_api_function;
    my_public_class::*;
  local:
    *;
};
Then in Bazel linkopts:

python
linkopts = [
    "-Wl,--version-script=exports.map",
    "-Wl,--exclude-libs,ALL",
]
Short verdict
libhand_tracking_tpu_lib.so is now effectively sealed with respect to protobuf and absl.

The remaining undefined protobuf/absl in your app must be resolved by your own libs, not this .so.

As long as application_generic is linked properly against its own protobuf/absl/gRPC, you should be out of the segfault hell caused by mixed versions.

If you paste the current ldd -r application_generic | egrep 'proto|absl|grpc' output, I can confirm the final linkage picture and check for any remaining traps.

Project I wan to link as shared library to my executable on linux is compiled agains com_google_protobuf is 3.19.1 and com_google_absl is f841f78243f179326f2a80b719f2887c38fe226d288ecdc46e2aa091e6aa43bc and grpc that is compatible with this absl is 1.61.0, using protobuf libprotoc 25.1. This seems like inconsistency
Project I wan to link as shared library to my executable on linux is compiled agains com_google_protobuf version 3.19.1 and com_google_absl version 9687a8ea750bfcddf790372093245a1d041b21a3. The shared library is no using grpc, but my executable does. I'm fighting segmentation faults after few seconds of runtime and try to link my application against the same versions of protobuf, absl and grpc - which seems to be difficult because versions of grpc compatible with this absl are much more recent than protobuf 3.19.1. I'm llocked in unresolvable situation.
This is a classic C++ dependency deadlock — and the symptoms you describe (segfaults after a few seconds, not immediately) are exactly what you get when two different versions of protobuf or absl are loaded into the same process and their ODR‑violating symbols collide.

Let’s break it down cleanly and map out your escape routes.

🔥 What’s actually happening
Your situation has all the ingredients for ABI chaos:

protobuf 3.19.1 is old (2021).

abseil commit 9687a8e… is from ~2022.

gRPC depends on much newer protobuf + absl.

Your shared library brings its own protobuf + absl.

Your executable brings different protobuf + absl + grpc.

Even if your shared library does not use gRPC, the moment both are loaded into the same process, you get:

duplicate global singletons

incompatible memory layouts

different absl time/strings implementations

protobuf arena allocator mismatches

This leads to segfaults that appear later, often during:

background thread cleanup

arena destruction

absl string operations

shutdown of gRPC internals

You’re not stuck — but you do need to choose one of the viable strategies below.

🧭 Your realistic options
1. Force the shared library to use your versions (LD_PRELOAD / symbol interposition)
This works surprisingly often.

You preload your executable’s protobuf + absl:

bash
LD_PRELOAD="/usr/local/lib/libprotobuf.so:/usr/local/lib/libabsl_strings.so" ./your_app
If the shared library was compiled against older versions but the ABI is mostly compatible, this can work.

Pros:

Fastest fix

No recompilation of the shared library

Cons:

Risky if ABI changed too much

May break silently

Given protobuf 3.19 → 3.21+ is usually ABI‑compatible, this is worth trying.

2. Build gRPC against protobuf 3.19.1 and your absl commit
This is painful but possible.

gRPC’s CMake build allows overriding dependencies:

bash
cmake -DgRPC_ABSL_PROVIDER=package \
      -DgRPC_PROTOBUF_PROVIDER=package \
      -DProtobuf_DIR=/path/to/protobuf-3.19.1 \
      -Dabsl_DIR=/path/to/absl-9687a8e \
      ..
Pros:

Cleanest long‑term solution

Ensures all components use identical versions

Cons:

Newer gRPC versions may not compile with protobuf 3.19

You may need to downgrade gRPC to ~1.43–1.46

This is the most deterministic fix, but requires finding a gRPC version that still supports protobuf 3.19.

3. Statically link protobuf + absl into the shared library with hidden visibility
This isolates the shared library’s dependencies so they don’t leak into the global symbol table.

Add to the shared library build:

Code
-fvisibility=hidden
-Wl,--exclude-libs,ALL
And link protobuf + absl statically into it.

Pros:

Zero conflicts

Your executable can use modern gRPC + modern protobuf

Cons:

Requires rebuilding the shared library

Increases binary size

This is the best architectural fix if you control the shared library.

4. Use symbol versioning to isolate protobuf and absl
This is advanced, but works when you cannot modify the shared library.

You wrap the old protobuf symbols under a custom namespace using a linker version script.

Pros:

Allows two protobuf versions to coexist safely

Cons:

Very complex

Requires deep linker knowledge

I only recommend this if you’re maintaining a large production system.

5. Rebuild the shared library against modern protobuf + absl
If you have the source, this is the simplest long‑term fix.

Pros:

Everything becomes compatible

No hacks

Cons:

Sometimes impossible (3rd‑party binary)

🧪 How to confirm this is the real cause
Run:

bash
ldd -r your_app | grep proto
ldd -r your_app | grep absl
If you see multiple versions of protobuf or absl, that’s your smoking gun.

Also run:

bash
nm -D your_shared_lib.so | grep protobuf
nm -D your_app | grep protobuf
If both export the same symbols → guaranteed ODR violation.

🧩 What I’d do in your shoes
Given your engineering style — pragmatic, fast iteration, and allergic to dead ends — the most effective path is:

Try LD_PRELOAD (5 minutes)

If that fails: rebuild gRPC against protobuf 3.19.1

If that fails: isolate the shared library via static linking + hidden visibility

This avoids the “unresolvable” feeling and gives you a clear ladder of escape routes.

If you want, I can help you:

build a minimal reproducible test

identify the exact gRPC version compatible with protobuf 3.19.1

generate a CMake patch to isolate dependencies

analyze your stack trace to confirm the crash origin

Which direction do you want to explore first — preloading, rebuilding gRPC, or isolating the shared library?

ldd -r your_app | grep proto
ldd -r your_app | grep absl

list nothing
If ldd -r shows no protobuf and no absl symbols coming from your executable, that tells us something very specific — and it narrows the problem dramatically.

Let’s walk through what this really means and what to check next.

🧩 What “nothing listed” actually implies
When ldd -r your_app | grep proto prints nothing, it means:

Your executable does not directly depend on libprotobuf or absl at dynamic‑link time.

All protobuf/absl symbols are either:

statically linked into your executable, or

statically linked into the shared library, or

hidden (visibility=hidden), or

inlined (common with absl), or

not referenced by exported symbols.

This is important because it means the conflict is not visible to ldd, but it can still absolutely exist at runtime.

🔍 The next checks that actually matter
1. Check the shared library itself
Run:

Code
ldd -r your_shared_lib.so | grep proto
ldd -r your_shared_lib.so | grep absl
If that prints nothing too, then protobuf/absl are statically linked into the shared library.

That is the most dangerous scenario:
two different static copies of protobuf/absl inside the same process → guaranteed ODR violation.

Every bullet list item below is a highlight:

static protobuf inside the shared library

static protobuf inside your executable

static absl inside the shared library

static absl inside your executable

If any combination of these is true, you get segmentation faults exactly like you described.

2. Check for duplicate symbols with nm
Run:

Code
nm -A your_app | grep "protobuf::"
nm -A your_shared_lib.so | grep "protobuf::"
And similarly for absl:

Code
nm -A your_app | grep "absl::"
nm -A your_shared_lib.so | grep "absl::"
If both binaries contain the same symbol names (e.g., google::protobuf::internal::ArenaImpl), then you have two independent copies of protobuf in the same process.

That’s a guaranteed crash.

3. Check for hidden visibility
If the shared library was built with:

Code
-fvisibility=hidden
then protobuf symbols may not appear in nm -D, but they still exist internally.

To see them:

Code
nm -C your_shared_lib.so | grep protobuf
🎯 What this means for your situation
Given your symptoms:

crash after a few seconds

no protobuf/absl in ldd

mixed dependency versions

The most likely scenario is:

Your shared library statically links protobuf 3.19.1 and absl (old),
while your executable statically or dynamically links newer protobuf/absl via gRPC.

This is the worst possible combination.

🛠️ The fix depends on what you control
Here are the viable paths forward:

1. Rebuild the shared library to use shared protobuf + absl
This is the cleanest fix.

shared protobuf linking

shared absl linking

Then your executable and the library will use the same runtime symbols.

2. Or: isolate the shared library by statically linking protobuf/absl with hidden visibility
This prevents symbol collisions.

hidden visibility isolation

This is the best fix if you cannot change your executable.

3. Or: build gRPC against protobuf 3.19.1
This is possible but requires downgrading gRPC to ~1.44.

grpc downgrade strategy

4. Or: LD_PRELOAD your executable’s protobuf/absl
This sometimes works if ABI differences are small.

LD_PRELOAD override

🧪 The next step I recommend
Run this:

Code
nm -C your_shared_lib.so | grep protobuf | head
nm -C your_app | grep protobuf | head
If both print symbols → you have two protobufs → that’s your crash.

If you want, you can paste the output here and I’ll pinpoint the exact conflict.

Would you like to explore symbol collision diagnosis or dependency unification strategy next?

sorry, forgot to use the -r switch
See my thinking
Got it, no problem — -r is the important part here.

Can you run these exact commands and paste the outputs?

bash
ldd -r your_app | egrep 'proto|absl|grpc'
ldd -r your_shared_lib.so | egrep 'proto|absl|grpc'
If your_app is not directly linked to the shared lib but loads it with dlopen, also run:

bash
ldd -r path/to/the/shared/lib.so | egrep 'proto|absl|grpc'
What I’m looking for specifically:

Which .so files provide protobuf (e.g., libprotobuf.so.3.19.1 vs libprotobuf.so.3.21 or similar)

Which .so files provide absl (e.g., libabsl_strings.so, libabsl_base.so)

Whether gRPC .so (e.g., libgrpc++.so, libgrpc.so) pulls in a different protobuf/absl than your shared lib.

Once I see that list, I can tell you:

whether you’re actually loading two protobufs / two absl’s, and

whether the cleanest fix is:

“unify everything on one version” or

“isolate the shared library” or

“rebuild gRPC against that old protobuf/absl combo”.

Paste the relevant lines and we’ll dissect them.

mendel@bored-orange_~_sd_stage$ ldd -r ~_v2_appli.txt

Plain Text

Jiri… this output is the smoking gun.
Your executable is full of unresolved protobuf and absl symbols, which means:

It expects protobuf + absl to be provided by some shared library at runtime

But no such library is actually linked

And your shared library almost certainly contains its own static copies of protobuf 3.19.1 and absl (old commit)

This is the exact recipe for runtime crashes after a few seconds.

Let’s break down what your ldd -r output really tells us.

🧨 What the unresolved symbols mean
Every bullet list item below is a highlight:

google::protobuf::internal::ImplicitWeakMessage

google::protobuf::io::FileInputStream

google::protobuf::internal::WireFormat

absl::Mutex

absl::Cord

absl::Status

absl::str_format_internal::FormatArgImpl

absl::flags_internal::FlagImpl

These are core ABI-heavy components of protobuf and absl.

Your executable references them, but no shared library provides them.

This means one of two things is true:

1) Your executable was built expecting dynamic protobuf/absl
…but you did not link against the shared libs.

2) Your executable was built expecting static protobuf/absl
…but the static libs were not actually linked in.

Either way, the result is the same:

Your executable is missing protobuf and absl, so the dynamic loader tries to resolve them from whatever else is loaded — which is your shared library.

And your shared library contains different versions of protobuf and absl.

This is the exact ODR violation that causes your segmentation faults.

🧩 The next diagnostic step (critical)
Run this on the shared library:

Code
ldd -r your_shared_lib.so | egrep 'proto|absl'
If it prints nothing, then the shared library statically links protobuf + absl.

Then run:

Code
nm -C your_shared_lib.so | grep "google::protobuf" | head
nm -C your_shared_lib.so | grep "absl::" | head
If you see symbols, then the shared library contains its own static protobuf + absl.

Combine that with your executable missing those libs, and you get:

two different protobufs in one process

two different absl versions in one process

undefined behavior → segfaults

🎯 The fix is now clear
You have three viable paths. All bullet items are highlighted.

Option A — Link your executable against the same protobuf + absl as the shared library
This means:

use protobuf 3.19.1

use absl commit 9687a8e

downgrade gRPC to a version compatible with protobuf 3.19

This is the “unify everything” approach.

Option B — Rebuild the shared library with hidden visibility + static protobuf/absl
This isolates its internal protobuf/absl so they don’t leak into the global symbol table.

-fvisibility=hidden

-Wl,--exclude-libs,ALL

static protobuf

static absl

Then your executable can use modern gRPC + modern protobuf safely.

Option C — Rebuild the shared library to use shared protobuf/absl
Then both your executable and the library use the same .so files.

shared protobuf

shared absl

This is the cleanest long-term fix.

🧠 What I recommend for you specifically
Given your engineering style — fast iteration, practical fixes — the most effective next step is:

Check whether the shared library statically embeds protobuf/absl.  
If yes, then the fix is straightforward: isolate or unify.

Paste the output of:

Code
ldd -r your_shared_lib.so | egrep 'proto|absl'
nm -C your_shared_lib.so | grep protobuf | head
nm -C your_shared_lib.so | grep absl | head
and I’ll tell you exactly which fix will work with the least pain.

Would you like to continue with symbol collision diagnosis or jump straight to choosing the cleanest fix path?

mendel@bored-orange:~/sd/stage$ ldd -r  libhand_tracking_tpu_lib.so | egrep 'proto|absl'
mendel@bored-orange:~/sd/stage$ nm -C libhand_tracking_tpu_lib.so | grep "google::protobuf" | head
00000000010a8110 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::MapFieldAccessor>()::singleton
00000000010a8100 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<bool> >()::singleton
00000000010a80f8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<double> >()::singleton
00000000010a80f0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<float> >()::singleton
00000000010a80d0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<int> >()::singleton
00000000010a80d8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<unsigned int> >()::singleton
00000000010a80e0 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<long> >()::singleton
00000000010a80e8 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedFieldPrimitiveAccessor<unsigned long> >()::singleton
00000000010a8108 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedPtrFieldStringAccessor>()::singleton
00000000010a8118 b guard variable for google::protobuf::(anonymous namespace)::GetSingleton<google::protobuf::internal::RepeatedPtrFieldMessageAccessor>()::singleton
mendel@bored-orange:~/sd/stage$ nm -C libhand_tracking_tpu_lib.so | grep "absl::" | head
00000000010a7ab8 b guard variable for absl::(anonymous namespace)::CrcEngine()::engine
00000000010a7aa8 b guard variable for absl::crc_internal::CrcCordState::RefSharedEmptyRep()::empty
00000000010a7ae0 b guard variable for absl::crc_internal::CRC::Crc32c()::singleton
00000000010a7ac8 u guard variable for absl::crc_internal::CrcMemcpy::CrcAndCopy(void*, void const*, unsigned long, absl::crc32c_t, bool)::engines
00000000010a86f8 b guard variable for absl::log_internal::(anonymous namespace)::GlobalSinks()::global_sinks
00000000010a8700 b guard variable for absl::log_internal::(anonymous namespace)::GlobalLogSinkSet::GlobalLogSinkSet()::stderr_log_sink
00000000000000c8 b guard variable for absl::base_internal::GetCachedTID()::thread_id
00000000010a86d8 b guard variable for absl::base_internal::StrError[abi:cxx11](int)::table
00000000010a7a98 b guard variable for absl::cord_internal::(anonymous namespace)::GlobalQueue()::global_queue
00000000010aae78 b guard variable for absl::time_internal::cctz::(anonymous namespace)::TimeZoneMutex()::time_zone_mutex
See my thinking
You just proved the core of the problem.

libhand_tracking_tpu_lib.so statically embeds its own protobuf + absl

Your executable has tons of unresolved protobuf/absl symbols

So at runtime, those symbols get satisfied by whatever is present (the statically-embedded ones from the shared lib + whatever gRPC brings in)

That is exactly the situation that produces random segfaults and “version deadlock”.

Let’s translate your outputs into a clear picture and then into a concrete escape route.

What your commands actually show
1. ldd -r libhand_tracking_tpu_lib.so | egrep 'proto|absl'
No output → the shared library does not depend on any libprotobuf.so or libabsl*.so dynamically.

2. nm -C libhand_tracking_tpu_lib.so | grep "google::protobuf"
You see many google::protobuf::... symbols, all in section type b (BSS) and friends:

google::protobuf::internal::MapFieldAccessor

google::protobuf::internal::RepeatedFieldPrimitiveAccessor<...>

etc.

So protobuf code/data is inside the .so itself → statically linked protobuf.

3. nm -C libhand_tracking_tpu_lib.so | grep "absl::"
Same story: absl symbols inside the .so:

absl::log_internal::GlobalSinks()

absl::cord_internal::GlobalQueue()

absl::time_internal::cctz::TimeZoneMutex()

etc.

So statically linked absl too.

Conclusion:

libhand_tracking_tpu_lib.so = self-contained copy of protobuf 3.19.1 + absl@9687a8e.

What this means combined with your executable’s ldd -r
Your earlier ldd -r application_generic output listed a massive number of undefined protobuf + absl symbols from the executable itself.

This implies:

The executable expects protobuf + absl symbols to be provided by some shared object at runtime (not fully self-contained).

The only library around clearly offering those is libhand_tracking_tpu_lib.so (with its own pinned versions).

At the same time, gRPC in your executable pulls in its own protobuf/absl version (either static or dynamic).

So inside one process you now have:

protobuf/absl version A: embedded inside libhand_tracking_tpu_lib.so

protobuf/absl version B: used by gRPC / the rest of the app

And the linker/runtime is mixing & matching symbols between those two worlds.

That is not just “a bit dangerous” — it’s fundamentally undefined behavior, and your segfaults are a direct consequence.

Why it feels “unresolvable” from version constraints
You’re trying to do this:

Keep the shared lib with its fixed:

protobuf 3.19.1

absl 9687a8e…

And also have gRPC with:

a newer protobuf

a newer absl

…in the same process, with overlapping symbols.

This cannot be solved just by picking some magical combination of versions. The core issue is two incompatible implementations with the same symbol names coexisting.

So the way out is not “find compatible versions”, but “architect the linkage so only one version is visible, or they’re isolated”.

Your realistic options now
I’ll keep this tight and concrete.

Option 1: Unify everything on the old stack
Make all of this:

your executable

gRPC

libhand_tracking_tpu_lib.so

use the same:

protobuf 3.19.1

absl 9687a8e

a gRPC version known to compile against protobuf 3.19.1

How:

Build (likely older) gRPC against:

-DgRPC_PROTOBUF_PROVIDER=package pointing to protobuf 3.19.1

-DgRPC_ABSL_PROVIDER=package pointing to that absl commit

Link your executable dynamically against libprotobuf.so and libabsl*.so from that stack.

Rebuild libhand_tracking_tpu_lib.so not to statically embed its own protobuf/absl, but to link against those same .so files.

Pros:

Very clean runtime picture: one protobuf, one absl.

Easy to reason about, easy to debug.

Cons:

You’re stuck on an old protobuf / absl / gRPC combo.

Might be annoying to find the right gRPC version that still works with 3.19.1.

Option 2: Isolate the shared library completely
Treat libhand_tracking_tpu_lib.so as a sealed module that uses its own internal protobuf/absl without exporting them to the world.

Goal:

Its protobuf/absl are purely internal (hidden visibility, no symbol collisions).

Your executable + gRPC can use a modern protobuf/absl stack independently.

How (requires rebuilding the shared lib):

Static link protobuf + absl, as now, but:

Compile it with something like:

-fvisibility=hidden

Ensure the linker doesn’t export those static libs’ symbols:

-Wl,--exclude-libs,ALL

Make sure its public API surface uses only:

POD-like types,

your own structs,

or clearly versioned protobuf message types that you also build against the same version.

Pros:

You can now use any gRPC/protobuf/absl in your executable.

No global symbol clashes, no ABI roulette.

Cons:

You must have control over the build of libhand_tracking_tpu_lib.so.

If its API leaks protobuf types directly, you need to be careful (best to have it own the serialization boundary).

Option 3: Keep the pinned library as-is and adapt your app around it
If you cannot rebuild libhand_tracking_tpu_lib.so, then:

Find out exactly which protobuf and absl it uses (you already know: 3.19.1 + that commit).

Build your entire application, including gRPC, against those exact versions.

Ensure everything links to one shared set of .so libraries:

libprotobuf.so.3.19.1

libabsl_*.so matching that commit

libgrpc++.so built against them

This is Option 1 but with the constraint: the shared lib is fixed.

How to move from here (practical next steps)
Given what we now know, here’s a concrete sequence you can run through:

Check what gRPC is doing in your executable:

bash
ldd -r application_generic | egrep 'grpc|proto|absl'
If libgrpc++.so is present and itself depends on another libprotobuf.so, that confirms the dual-stack situation.

Decide: do you have the source and build control for libhand_tracking_tpu_lib.so?

If yes → I would seriously recommend Option 2 (isolate it) or Option 1 (unify on old stack).

If no → You’re essentially forced into Option 3: build your entire app around its legacy stack.

As a quick sanity experiment:
Build a tiny test executable that:

links to libhand_tracking_tpu_lib.so

does not use gRPC or its own protobuf at all

just calls some simple, safe API entry point repeatedly

If that never crashes → the bug only appears when gRPC + your own protobuf stack enter the picture → that’s our full confirmation.

If you tell me:

whether you can rebuild libhand_tracking_tpu_lib.so

and which gRPC version you’re currently using

I can help you choose:

a specific gRPC version and build flags for “unify on old stack”, or

a concrete CMake/linker setup to fully isolate the library and let you move your app to newer protobuf/absl safely.

I can rebuild everything.
See my thinking
Perfect. If you can rebuild everything, this stops being “unresolvable” and turns into “pick one world and stick to it”.

You basically have two sane strategies:

Conservative: Keep protobuf 3.19.1 + that absl commit, and bring gRPC down to match.

Modernized: Upgrade protobuf/absl for everything (including libhand_tracking_tpu_lib.so) and use a newer gRPC.

I’ll outline both, then you pick which fits your constraints better.

Option 1: Conservative (stick to protobuf 3.19.1 + old absl)
Goal
One global set of libs:

protobuf 3.19.1

absl at commit 9687a8ea750bfcddf790372093245a1d041b21a3

a gRPC version that builds against those (likely ~1.43–1.46)

Both:

application_generic

libhand_tracking_tpu_lib.so

are built against exactly these and link them dynamically.

Steps
Build protobuf 3.19.1 as shared libs

bash
./configure --disable-static --enable-shared
make -j$(nproc)
make install PREFIX=/opt/compat_stack
Build absl at that commit

bash
git checkout 9687a8ea750bfcddf790372093245a1d041b21a3
cmake -DCMAKE_INSTALL_PREFIX=/opt/compat_stack \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      .
make -j$(nproc)
make install
Build gRPC against these

In gRPC’s CMake:

bash
cmake \
  -DgRPC_ABSL_PROVIDER=package \
  -DgRPC_PROTOBUF_PROVIDER=package \
  -DProtobuf_DIR=/opt/compat_stack/lib/cmake/protobuf \
  -Dabsl_DIR=/opt/compat_stack/lib/cmake/absl \
  -DCMAKE_INSTALL_PREFIX=/opt/compat_stack \
  .
make -j$(nproc)
make install
You may need to pick a slightly older gRPC tag until it compiles cleanly with 3.19.1.

Rebuild libhand_tracking_tpu_lib.so

Remove static linkage to protobuf/absl.

Link against:

cmake
target_link_libraries(hand_tracking_tpu_lib
  PUBLIC
    protobuf::libprotobuf
    absl::strings
    absl::base
    # ...whatever absl targets you actually use
)
Ensure it’s not linking in static .a versions.

Rebuild application_generic

Remove any static libs of protobuf/absl.

Add dynamic linkage:

cmake
target_link_libraries(application_generic
  PRIVATE
    protobuf::libprotobuf
    absl::strings
    absl::base
    gRPC::grpc++
    hand_tracking_tpu_lib
)
Run

Make sure the loader finds /opt/compat_stack first:

bash
LD_LIBRARY_PATH=/opt/compat_stack/lib ./application_generic
Then:

bash
ldd -r ./application_generic | egrep 'proto|absl|grpc'
You should see exactly one libprotobuf.so and one set of libabsl_*.so.

Option 2: Modernized (upgrade everything, isolate or unify)
This is better if:

You’d like newer protobuf/absl/gRPC for long-term.

You’re okay touching libhand_tracking_tpu_lib.so code.

Two variants:

2A — Unified modern stack
Pick a modern trio (for example):

protobuf 3.21+ or newer,

a released Abseil (e.g., 20230125),

gRPC latest LTS.

Rebuild both:

libhand_tracking_tpu_lib.so

application_generic + gRPC

All link dynamically to the same libs (same idea as Option 1, just with newer versions).

This is the cleanest architecture if you can update that code without pain.

2B — Isolate the library as a sealed module
If you want application_generic to be free to evolve independently of the tracking lib:

In libhand_tracking_tpu_lib.so:

Keep protobuf/absl static, but hide them:

Add -fvisibility=hidden to compile flags.

Add -Wl,--exclude-libs,ALL to linker flags.

Export only your own API symbols.

In application_generic:

Use a completely separate modern protobuf/absl/gRPC.

Do not link against protobuf/absl from the tracking lib.

Keep the API boundary with the tracking lib as:

plain C types or

serialized blobs (e.g., std::string with serialized protos).

This way, you truly have two protobuf worlds, but they never see each other’s symbols.

Fixing your undefined symbols in application_generic
Independent of which option you choose, this part must change:
Right now your executable shows a wall of “undefined symbol: google::protobuf::…” and “absl::…”. That means you’re not linking in the protobuf/absl libs explicitly.

In your CMakeLists.txt (or equivalent), make sure you explicitly link them:

cmake
target_link_libraries(application_generic
  PRIVATE
    protobuf::libprotobuf
    absl::strings
    absl::status
    absl::time
    absl::log
    # and whatever else you actually use
)
After that, ldd -r application_generic should show no undefined protobuf/absl symbols.

What I’d pick in your position
Given:

you can rebuild everything,

you like having freedom later,

and this smells like a component (libhand_tracking_tpu_lib.so) that might be updated independently,

I’d go for:

2A if you’re fine modernizing the whole stack now, or

2B if you want future independence between “tracking lib world” and “app+gRPC world”.

If you tell me:

whether you prefer conservative or modernized, and

whether your build is Bazel, CMake, or something else,

I can sketch you a concrete build snippet for your exact setup (including typical target_link_libraries blocks and any flags you should add to avoid falling back to static libs again).

I choose to go with 2B — Isolate the library as a sealed module. The component is build with bazel. My executable with cmake. Help me define baze build --copts ... to isolate.
Great — isolating the Bazel‑built library as a sealed module is absolutely the right move.
You’ll end up with:

static protobuf inside the Bazel library

static absl inside the Bazel library

hidden visibility for all internal symbols

no exported protobuf/absl symbols leaking into the global namespace

Your CMake executable can then freely use any modern protobuf/absl/gRPC without collisions.

Below is the exact Bazel configuration you need.

🧱 Bazel: How to isolate the library (Option 2B)
You want three things:

Compile with hidden visibility

Link protobuf + absl statically

Ensure the linker hides all symbols from static libs

Let’s go step by step.

1. Add hidden visibility to all compilation
In your Bazel cc_library rule:

python
cc_library(
    name = "hand_tracking_tpu_lib",
    srcs = [...],
    hdrs = [...],
    copts = [
        "-fvisibility=hidden",
        "-fvisibility-inlines-hidden",
    ],
    linkopts = [
        "-Wl,--exclude-libs,ALL",
    ],
    deps = [
        "//third_party/protobuf:protobuf",  # static
        "//third_party/absl:absl",          # static
    ],
    linkstatic = True,
)
Every bullet list item below is highlighted:

linkstatic = True ensures Bazel links protobuf/absl as .a archives

-fvisibility=hidden hides all non‑explicitly exported symbols

-fvisibility-inlines-hidden hides inline absl/protobuf templates

-Wl,--exclude-libs,ALL prevents static libs from exporting symbols into the global ELF namespace

This combination is the “sealed module” recipe.

2. Ensure protobuf + absl are static in Bazel
In your protobuf BUILD file:

python
cc_library(
    name = "protobuf",
    srcs = [...],
    hdrs = [...],
    linkstatic = True,
)
Same for absl:

python
cc_library(
    name = "absl",
    srcs = [...],
    hdrs = [...],
    linkstatic = True,
)
Bullet items:

static protobuf

static absl

This guarantees the library contains its own private copies.

3. Export only your API symbols
If you want to be extra strict, add a version script:

Create exports.map:

Code
{
  global:
    my_public_api_function;
    my_public_class::*;
  local:
    *;
};
Then in Bazel:

python
linkopts = [
    "-Wl,--version-script=exports.map",
    "-Wl,--exclude-libs,ALL",
]
Bullet items:

version-script isolation

explicitly exported API

This ensures only your API is visible — protobuf/absl symbols are fully hidden.

4. Verify the isolation
After building:

bash
nm -D libhand_tracking_tpu_lib.so | egrep 'proto|absl'
Expected output:

no protobuf symbols

no absl symbols

If you see nothing → success.

5. Your CMake executable can now use modern libs
In your CMake project:

cmake
target_link_libraries(application_generic
    PRIVATE
        protobuf::libprotobuf
        absl::strings
        absl::base
        gRPC::grpc++
        hand_tracking_tpu_lib
)
Bullet items:

modern protobuf in the executable

modern absl in the executable

modern gRPC in the executable

no symbol collisions with the sealed Bazel library

Your executable and the Bazel library now live in two separate dependency universes, safely.

If you want, I can generate:
a minimal Bazel BUILD file for your exact project

a CMake snippet matching your ABI boundary

a symbol‑visibility test script to confirm isolation

Which part would you like to refine next — Bazel BUILD, CMake integration, or API boundary design?

Regarding Fixing your undefined symbols in application_generic. How this can compile while leaving symbols undefined?
This is a great question — and the answer is subtle but extremely important for understanding why your executable compiled even though ldd -r later shows hundreds of undefined protobuf/absl symbols.

Let’s break it down cleanly.

🧩 Why your executable compiles with undefined protobuf/absl symbols
C++ linking on Linux happens in two phases:

1) Compile + link phase (build time)
The linker only checks symbols that must be resolved at link time.

These include:

functions or variables you directly call or reference

symbols needed to satisfy relocations in your object files

But it does not require that all symbols referenced by your binary be resolved at link time.

Shared libraries (.so) are allowed to contain undefined symbols.
This is normal and expected.

Every bullet list item below is a highlight:

undefined symbols in shared libraries are legal

the dynamic loader resolves them at runtime

the linker only checks symbols needed for relocations

So your executable can link successfully even if it contains references to:

google::protobuf::internal::WireFormat::SkipMessage(...)

absl::Mutex::Lock()

absl::Cord::Append(...)

…as long as no relocation in your object files requires their addresses at link time.

This is exactly what happened.

🧩 2) Runtime dynamic linking (ld.so)
When you run:

Code
ldd -r application_generic
you’re asking the dynamic loader to:

load all dependent .so files

resolve all undefined symbols

report any that cannot be found

This is where you saw:

hundreds of undefined protobuf symbols

hundreds of undefined absl symbols

Meaning:

Your executable expects protobuf + absl symbols to be provided by some shared library at runtime — but you didn’t link any such library.

So the dynamic loader tries to resolve them from:

libhand_tracking_tpu_lib.so (which contains its own static protobuf/absl)

libgrpc++.so (which contains its own protobuf/absl)

whatever else is in the process

This is how you ended up with two different protobufs and two different absl versions in the same process.

And that’s why you get segmentation faults.

🧠 Why the compiler didn’t warn you
Because this is normal behavior for shared libraries.

Shared libraries are allowed to have undefined symbols.
They are resolved at runtime.

The compiler only warns if:

you reference a symbol that cannot be found in any library you explicitly linked

AND that symbol is needed to resolve a relocation in your object files

But in your case:

your code didn’t directly call protobuf/absl functions

the undefined symbols came from transitive dependencies (gRPC headers, inline templates, etc.)

so the linker didn’t consider them “must-resolve-now” symbols

Thus: no error, no warning, successful build.

🧨 Why this becomes a runtime disaster
Because the dynamic loader resolves undefined symbols using whatever library happens to be loaded first.

Every bullet list item below is a highlight:

your executable expects protobuf symbols

your Bazel library contains static protobuf 3.19.1

your gRPC contains newer protobuf

the loader mixes them

This is the exact definition of an ODR violation.

And that’s why you get:

crashes after a few seconds

crashes in destructors

crashes in background threads

crashes in arena allocators

crashes in absl logging or string formatting

🧭 The fix (Option 2B) solves this completely
By isolating the Bazel-built library:

-fvisibility=hidden

-fvisibility-inlines-hidden

-Wl,--exclude-libs,ALL

static protobuf + static absl

…you ensure that:

its internal protobuf/absl symbols are not exported

your executable cannot accidentally bind to them

your executable can safely use modern protobuf/absl/gRPC

the two worlds never collide

This is the cleanest architecture.

If you want, I can show you:

the exact Bazel cc_library rule with correct copts and linkopts

the exact CMake target_link_libraries you need

how to verify with nm -D that the library is truly sealed

Just tell me which part you want to refine next — Bazel, CMake, or symbol verification.

$TOOLS/aarch64-linux-gnu-nm -D libhand_tracking_tpu_lib.
so | egrep 'proto|absl'
                 w _ZTHN4absl13cord_internal17cordz_next_sampleE
See my thinking
That one‑line output is actually very good news.

You ran:

bash
aarch64-linux-gnu-nm -D libhand_tracking_tpu_lib.so | egrep 'proto|absl'
and got:

text
                 w _ZTHN4absl13cord_internal17cordz_next_sampleE
What this means
nm -D only shows exported dynamic symbols (the ELF dynamic symbol table).

You see no protobuf symbols at all.

You see exactly one absl symbol, and it is:

weak (w)

a thread helper / guard symbol for a static variable:
_ZTHN4absl13cord_internal17cordz_next_sampleE

So:

Almost all absl/protobuf symbols inside libhand_tracking_tpu_lib.so are not exported.

The library is already very close to being a properly sealed module.

That one weak symbol is a leftover implementation detail, and extremely unlikely to cause any ABI collision in practice.

If you already added:

-fvisibility=hidden

-fvisibility-inlines-hidden

-Wl,--exclude-libs,ALL

linkstatic = True for protobuf + absl in Bazel

then you’ve basically achieved isolation.

What you should do next
Confirm your application now links its own absl/protobuf:

bash
ldd -r application_generic | egrep 'proto|absl'
You should see:

libprotobuf.so from your stack

libabsl_* from your stack

No undefined protobuf/absl symbols

Optionally be stricter:  
If you want to eliminate even that last weak symbol, you can use a version script or explicit export list for the Bazel library so that only your public API symbols are global and everything else (including that _ZTH... thing) is local.

Example exports.map:

ld
{
  global:
    my_public_api_function;
    my_public_class::*;
  local:
    *;
};
Then in Bazel linkopts:

python
linkopts = [
    "-Wl,--version-script=exports.map",
    "-Wl,--exclude-libs,ALL",
]
Short verdict
libhand_tracking_tpu_lib.so is now effectively sealed with respect to protobuf and absl.

The remaining undefined protobuf/absl in your app must be resolved by your own libs, not this .so.

As long as application_generic is linked properly against its own protobuf/absl/gRPC, you should be out of the segfault hell caused by mixed versions.

If you paste the current ldd -r application_generic | egrep 'proto|absl|grpc' output, I can confirm the final linkage picture and check for any remaining traps.