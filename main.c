#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mscm.h"
#include "dump.h"

static bool ends_with(char const *str, char const *suffix);
static bool is_empty_line(char const *str);
static bool strcmp_spaceignore(char const *input, char const *test);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <files...>\n", argv[0]);
        return 1;
    }

    mscm_engine *engine = mscm_engine_create();
    if (!engine) {
        fprintf(stderr, "error: could not create engine\n");
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (ends_with(argv[i], ".dll") || ends_with(argv[i], ".so")) {
            if (!mscm_engine_load_ext(engine, argv[i])) {
                fprintf(stderr,
                        "error: could not load extension %s\n",
                        argv[i]);
            }
        }
        else if (!strcmp(argv[i], "--repl")) {
            if (i != argc - 1) {
                fprintf(stderr,
                        "error: --repl must be the last argument\n");
                continue;
            }

            char inbuf[4096];
            while (true) {
                fprintf(stderr, "mini-scheme> ");
                fflush(stdout);
                if (!fgets(inbuf, sizeof(inbuf), stdin)
                    || strcmp_spaceignore(inbuf, "exit")
                    || strcmp_spaceignore(inbuf, "quit")) {
                    fprintf(stderr, "\nMoriturus te saluto.\n");
                    break;
                }

                if (strlen(inbuf) == sizeof(inbuf) - 1) {
                    fprintf(stderr,
                            "error: input too long\n");
                    continue;
                }

                if (is_empty_line(inbuf)) {
                    continue;
                }

                mscm_value ret =
                    mscm_engine_eval_string(engine, "<stdin>", inbuf);
                if (ret) {
                    mscm_value_dump(ret);
                    putchar('\n');
                    fflush(stdout);
                }
            }
        }
        else {
            mscm_engine_eval_file(engine, argv[i]);
        }
    }

    mscm_engine_destroy(engine);
    return 0;
}

static bool ends_with(char const *str, char const *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) {
        return false;
    }
    return !strcmp(str + str_len - suffix_len, suffix);
}

static bool is_empty_line(char const *str) {
    while (*str) {
        if (!isspace(*str)) {
            return false;
        }
        ++str;
    }
    return true;
}

static bool strcmp_spaceignore(char const *input, char const *test) {
    while (input[0] && isspace(input[0])) {
        ++input;
    }

    while (test[0]) {
        if (input[0] != test[0]) {
            return false;
        }
        ++input;
        ++test;
    }

    while (input[0] && isspace(input[0])) {
        ++input;
    }

    return !input[0];
}
