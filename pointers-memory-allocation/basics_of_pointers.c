// Declare an integer variable and initialize it with a value.
// Declare a pointer variable that points to the integer.
// Print the address of the integer variable using both the variable and the pointer.
// Modify the value of the integer using the pointer and print the new value.

#include <stdio.h>

int main() {
    int target_value = 42;
    int *ptr = &target_value;

    printf("Address via address-of operator (&target_value): %p\n", &target_value);
    printf("Address via pointer variable (ptr):              %p\n", ptr);

    *ptr = 99;
    printf("New value of target_value: %d\n", target_value);

    return 0;
}
