// Declare an array of integers and initialize it with 5 values.
// Use a pointer to traverse the array and print each element.
// Modify the values of the array using pointer arithmetic.
// Print the modified array using both the pointer and the array name.

#include <stdio.h>

int main() {
    int numbers[5] = {10, 20, 30, 40, 50};

    int *ptr = numbers;

    printf("Original array elements:\n");
    for (int i = 0; i < 5; i++) {
        printf("Element %d: %d\n", i, *(ptr + i));
    }

    for (int i = 0; i < 5; i++) {
        *(ptr + i) += 5;
    }

    printf("\nModified array (Comparing notations):\n");
    for (int i = 0; i < 5; i++) {
        printf("numbers[%d] = %d | *(ptr + %d) = %d\n",
                i, numbers[i], i, *(ptr + i));
    }

    return 0;
}
