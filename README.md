# ShatalovBehavior
> Developed with **Unreal Engine 4.27**

This is a super very basic task system that was used in the Hello Neighbor game from Prototype to Release. Originally developed by **Anton Shatalov**.
The system is created on the **UGameplayTask** class, instead of UObject as in Hello Neighbor (due to the absence of UGameplayTask in UE4.8).
## Project
The project includes a founder class (as AActor) and **UBehavior** itself.

#### UBehavior has the following main functions:
- **RunBehavior** - Running a new task
- **FinishBehavior** - Finishes the task
- **SetIsInterrupted** - Prevents all tasks from finishing before the task (including the current one) in which IsInterrupted was set
- **Ready** - Starts the task if it's not started yet.
- **BehStart** (Activate)
- **BehTick** (TickTask)

UBehavior also includes a **prioritization system**, where lower priority tasks are queued and high priority tasks overwrite other tasks.

> Some of my ideas may not coincide with Anton Shatalov's ideas, for example, I didn't find an explanation for IsInterrupted in Hello Neighbor, so I rethought this variable for my project.
