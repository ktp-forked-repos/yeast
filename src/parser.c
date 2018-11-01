#include "interface.h"
#include "yeast.h"
#include "parser.h"

YEAST_DOC(make_parser, "LANGUAGE",
          "Make a new parser object for the given LANGUAGE.");
emacs_value yeast_make_parser(emacs_env *env, emacs_value language)
{
    YEAST_ASSERT_SYMBOL(language);
    return em_nil;
}
