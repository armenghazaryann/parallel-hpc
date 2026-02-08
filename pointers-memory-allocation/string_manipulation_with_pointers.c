// Declare a character pointer and assign it a string literal.
// Use a pointer to traverse and print the string character by character.
// Write a function str_length(char *str) that calculates the length of a string using pointer arithmetic.
// Call str_length() in main() and print the length of a user-provided string.

#include <stdio.h>

int str_length(char *str) {
    char *ptr = str;

    while (*ptr != '\0') {
        ptr++;
    }

    return ptr - str;
}

int main() {
    char *message = "Hello, System!";
    char *scanner = message;

    printf("Traversing string literal:\n");
    while (*scanner != '\0') {
        printf("'%c' ", *scanner);
        scanner++;
    }
    printf("\n\n");

    char user_input[100];

    printf("Enter a string (no spaces): ");
    scanf("%99s", user_input);

    printf("Length of your string '%s' is: %d\n",
           user_input, str_length(user_input));

    return 0;
}
