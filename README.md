# ECE-721-Proj-3_Superscalar_Processor_Pipeline_Simulator
This project extends a cycle-accurate superscalar RISC-V pipeline simulator (721sim) by completing missing pipeline components and integrating a previously developed register renaming module. The simulator models the canonical superscalar pipeline taught in ECE 721: Advanced Microarchitecture.

Features
  Full Pipeline Simulation:  Frontend: Fetch, Decode, Rename, Dispatch
                             Backend: Schedule, Register Read, Execute, Writeback, Retire
  Explicit Modeling:         Pipeline registers, queues, issue queue, execution lanes, and branch handling are fully represented.
  Renamer Integration:       Added register renaming support from Project 2 for managing a unified physical register file.
  Completed Missing Logic:   Implemented intentionally omitted functionality across seven files (rename.cc, dispatch.cc, register_read.cc,                                  execute.cc, writeback.cc, retire.cc, squash.cc).
  Validation & Debugging:    Compared simulator output against a functional simulator and instructor-provided benchmarks to verify correctness.

Implementation Tasks
  Integrated the renamer module into the simulator pipeline.
  Implemented ~18 missing code segments for:
      Rename and Dispatch stages
      Register Read, Execute, and Writeback
      Retire and squash mechanisms for precise exception handling
      Ensured correct instruction flow through pipeline registers and queues.
      Verified correctness with functional checking and performance metrics (IPC).
  
  Outputs & Evaluation
      Runs full RISC-V instruction traces.
      Reports correctness through internal assertions and a functional checker.
      Performance measured using Instructions Per Cycle (IPC) compared to reference runs.
      
  Key Learnings
      Gained deep experience in building and debugging a cycle-accurate superscalar simulator.
      Learned how pipeline resources (issue queue, physical registers, Active List, LSU) interact to sustain instruction throughput.
      Understood how precise exceptions, squashing, and register renaming are managed in out-of-order pipelines.
      Strengthened skills in C++ system-level modeling, debugging, and performance validation.
