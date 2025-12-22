#define DS_IMPLEMENTATION
#include "ds.h"

int main() {
    string_builder builder = sb_make("hello world!\n", 0);

    string str = sb_append(&builder, "bye!\n");

    str_print(str);
	string s = str_make("Hello, world!\n");
	str_print(s);
	return 0;
}
