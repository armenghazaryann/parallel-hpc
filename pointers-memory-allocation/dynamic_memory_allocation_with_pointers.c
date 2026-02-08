// Use malloc() to allocate memory for an integer.
// Assign a value to the allocated memory and print it.
// Use malloc() to allocate memory for an array of 5 integers.
// Populate the array using pointer arithmetic and print the values.
// Free all allocated memory.

#include <stdio.h>
#include <stdlib.h>

int main() {
    int *single_ptr = malloc(sizeof(int));

    *single_ptr = 42;
    printf("Value in dynamic integer: %d\n", *single_ptr);

    int count = 5;
    int *array_ptr = malloc(count * sizeof(int));

    printf("Dynamic Array Values:\n");
    for (int i = 0; i < count; i++) {
        *(array_ptr + i) = (i + 1) * 10;
        printf("%d ", *(array_ptr + i));
    }
    printf("\n");

    free(single_ptr);
    free(array_ptr);

    printf("Memory freed successfully.\n");

    return 0;
}
