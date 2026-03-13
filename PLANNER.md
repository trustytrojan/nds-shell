## if you want to protect the "kernel" memory
we need to implement a system call dispatcher, because otherwise CPSR can't change.
so i think it's best to be less of a secure kernel for now: i want to get executables working at the very least
and we need to define "tasks" in a similar fashion to how linux does it

## really nice example from gemini
```c
typedef struct TaskControlBlock {
    // 1. Execution Context
    cothread_t thread_handle;   // The hardware library's thread object
    void *stack_base;           // Pointer to the start of this thread's stack
    size_t stack_size;

    // 2. Identity (The Linux "Process" feel)
    int tid;                    // Task ID
    int ptid;                   // Parent Task ID (who spawned this?)
    
    // 3. Process Resources (The "sharing" logic)
    // For a 'Thread', these point to the parent's data.
    // For a 'Process', these point to newly allocated memory.
    char **argv;                // Pointer to the argument array
    char **envp;                // Pointer to the environment array
    mspace heap_space;          // The dlmalloc 'instance' for this task
    
    // 4. State Management
    bool is_running;
    int exit_code;
} TCB_t;
```

we **can** allow programs to have an `int main(int argc, char **argv)`!
- we need to somehow override the DSL's `setenv` and `getenv` to operate on the 

# LOOK HERE

we can change libnds to provide a callback API to applications to intercept any symbols a DSL needs to be resolved!
this gives us the power to fill in a DSL's `setenv` or `getenv` with our own impl! 

next challenge: how do we identify *which* task made our new "system calls"?
would a software interrupt into "kernel" code, along with getting the current thread's ID (which we can define), be enough?
must experiment to find out.
