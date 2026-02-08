// Declare an integer variable and a pointer to that variable.
// Declare a pointer to the pointer and initialize it.
// Print the value of the integer using both the pointer and the double-pointer.

#include <stdio.h>

int main() {
    int target_value = 555;
    int *single_ptr = &target_value;

    int **double_ptr = &single_ptr;

    printf("Value via variable:       %d\n", target_value);
    printf("Value via single pointer: %d\n", *single_ptr);
    printf("Value via double pointer: %d\n", **double_ptr);

    return 0;
}
