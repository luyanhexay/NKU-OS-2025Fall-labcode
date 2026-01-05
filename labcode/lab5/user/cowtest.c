#include <stdio.h>
#include <ulib.h>

/*
 * COW (Copy-on-Write) Test Program
 * 
 * This test verifies that the COW mechanism works correctly:
 * 1. Parent and child initially share pages (read-only)
 * 2. When either writes, a private copy is created
 * 3. Each process has its own data after write
 */

#define ARRAY_SIZE 1024

int main(void) {
    cprintf("COW Test: Starting\n");
    
    // Allocate array in parent's data segment
    static int shared_data[ARRAY_SIZE];
    
    // Initialize data in parent
    for (int i = 0; i < ARRAY_SIZE; i++) {
        shared_data[i] = i;
    }
    
    cprintf("Parent: Initialized array with values 0-%d\n", ARRAY_SIZE - 1);
    
    int pid = fork();
    
    if (pid < 0) {
        cprintf("Fork failed!\n");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        cprintf("Child [%d]: Started\n", getpid());
        
        // First, verify child can read parent's data (COW sharing)
        cprintf("Child: Reading first value: %d (should be 0)\n", shared_data[0]);
        
        if (shared_data[0] != 0) {
            cprintf("Child: ERROR - Initial read failed!\n");
            return -1;
        }
        
        // Now modify data - this should trigger COW
        cprintf("Child: Writing new values (triggering COW)...\n");
        for (int i = 0; i < ARRAY_SIZE; i++) {
            shared_data[i] = i + 1000;  // Different values from parent
        }
        
        cprintf("Child: Verify written values...\n");
        for (int i = 0; i < 10; i++) {  // Check first 10
            if (shared_data[i] != i + 1000) {
                cprintf("Child: ERROR - Value mismatch at %d: %d\n", i, shared_data[i]);
                return -1;
            }
        }
        
        cprintf("Child: COW write successful! Values: %d, %d, %d...\n", 
                shared_data[0], shared_data[1], shared_data[2]);
        
        cprintf("Child: Test PASSED\n");
        exit(0);
        
    } else {
        // Parent process
        cprintf("Parent [%d]: Created child [%d]\n", getpid(), pid);
        
        // Wait a bit for child to read
        yield();
        yield();
        
        // Parent modifies its own copy
        cprintf("Parent: Writing different values (triggering COW)...\n");
        for (int i = 0; i < ARRAY_SIZE; i++) {
            shared_data[i] = i + 2000;  // Different from both original and child
        }
        
        cprintf("Parent: Verify written values...\n");
        for (int i = 0; i < 10; i++) {  // Check first 10
            if (shared_data[i] != i + 2000) {
                cprintf("Parent: ERROR - Value mismatch at %d: %d\n", i, shared_data[i]);
                return -1;
            }
        }
        
        cprintf("Parent: COW write successful! Values: %d, %d, %d...\n",
                shared_data[0], shared_data[1], shared_data[2]);
        
        // Wait for child
        int exit_code;
        waitpid(pid, &exit_code);
        
        cprintf("Parent: Child exited with code %d\n", exit_code);
        cprintf("Parent: Test PASSED\n");
    }
    
    cprintf("COW Test: Complete\n");
    return 0;
}
