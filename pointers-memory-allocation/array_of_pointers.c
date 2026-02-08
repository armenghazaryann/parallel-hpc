// Declare an array of strings using an array of character pointers.
// Print each string using pointer notation.
// Modify one of the strings and print the updated array.

#include <stdio.h>

int main() {
    char *team_members[] = {
        "Alice",
        "Bob",
        "Charlie",
        "David"
    };

    int count = 4;

    printf("--- Original Team ---\n");

    for (int i = 0; i < count; i++) {
        printf("Member %d: %s\n", i, *(team_members + i));
    }

    *(team_members + 1) = "Robert";

    printf("\n--- Updated Team ---\n");

    for (int i = 0; i < count; i++) {
        printf("Member %d: %s\n", i, *(team_members + i));
    }

    return 0;
}
