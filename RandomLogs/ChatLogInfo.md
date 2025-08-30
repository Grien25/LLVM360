Here’s the snapshot from the logs.

# What LLVM360 can do right now (per the chats)

* The runtime **supports external function calls**, which was called out as the “next step to get something interesting to run.”&#x20;
* It has logic in flight for **indirect/virtual calls**: the generated EXE exports an array of `{xex_32bit_address → recompiled_fn_ptr}`, and the runtime resolves targets at call time.&#x20;
* During recompilation it **dumps XEX sections to .bin** so the runtime can load/allocate them at the right virtual addresses (rather than baking data into the EXE). &#x20;
* There’s attention to **addressing & perf** details (e.g., using an `r15 + offset` base to make data access cheap).&#x20;
* Constraints called out during experiments: **64 KB alignment** is required and **forcing virtual addresses** can “mess up everything,” so the loader/mapper still needs care. &#x20;

Reality check on current momentum: when asked “How’s LLVM360 going?” on Aug 28, 2025, the answer was **“basically nothing, been doing other stuff recently”** (and “idk when I’ll be back to this”). So progress appears paused at the moment. &#x20;

# What’s next (from the chats & implied by the work-in-progress)

* **Finish / harden the indirect-call path** (faster lookup than linear search; robust mapping export).&#x20;
* **Implement more PPC instructions** in the recompiler and **improve the function-bound analyser** (explicitly listed as upcoming work).&#x20;
* **Stabilize the runtime loader**: load section .bins at correct virtual addrs, honor **64 KB alignment**, and avoid corruption when addresses are forced.  &#x20;
* **Target “something interesting to run”** end-to-end (the near-term milestone mentioned alongside external-call support).&#x20;
* Longer-term vision in the chat: a flow where you **launch a game/app → if not yet recompiled, recompile → cache → run** (i.e., a one-click recompiler+runtime pipeline).&#x20;

If you want, I can turn these into a tight engineering checklist (owner, success criteria, test artifact) you can drop into an issue tracker.
