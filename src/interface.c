#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "emacs-module.h"
#include "interface.h"

#define GLOBREF(val) env->make_global_ref(env, (val))
#define INTERN(val) env->intern(env, (val))

// We store some global references to emacs objects, mostly symbols,
// so that we don't have to waste time calling intern later on.
emacs_value em_nil, em_t;
emacs_value em_stringp, em_symbolp;
emacs_value em_yeast_instance_p;

// Error symbols
emacs_value em_unknown_language;

// Supported languages
emacs_value em_json, em_python;

// Symbols that are only reachable from within this file.
static emacs_value _buffer_size, _buffer_substring, _cons, _defalias,
    _point_min, _point_max, _provide, _user_ptrp, _wrong_type_argument;

void em_init(emacs_env *env)
{
    em_nil = GLOBREF(INTERN("nil"));
    em_t = GLOBREF(INTERN("t"));

    em_stringp = GLOBREF(INTERN("stringp"));
    em_symbolp = GLOBREF(INTERN("symbolp"));

    em_yeast_instance_p = GLOBREF(INTERN("yeast-instance-p"));

    em_unknown_language = GLOBREF(INTERN("unknown-language"));

    em_json = GLOBREF(INTERN("json"));
    em_python = GLOBREF(INTERN("python"));

    _buffer_size = GLOBREF(INTERN("buffer-size"));
    _buffer_substring = GLOBREF(INTERN("buffer-substring"));
    _cons = GLOBREF(INTERN("cons"));
    _defalias = GLOBREF(INTERN("defalias"));
    _point_min = GLOBREF(INTERN("point-min"));
    _point_max = GLOBREF(INTERN("point-max"));
    _provide = GLOBREF(INTERN("provide"));
    _user_ptrp = GLOBREF(INTERN("user-ptrp"));
    _wrong_type_argument = GLOBREF(INTERN("wrong-type-argument"));
}

/**
 * Call an Emacs function without error checking.
 * @param env The active Emacs environment.
 * @param func The function to call.
 * @param nargs The number of arguments that follow.
 * @return The function return value.
 */
static emacs_value em_funcall(emacs_env *env, emacs_value func, ptrdiff_t nargs, ...)
{
    emacs_value args[nargs];

    va_list vargs;
    va_start(vargs, nargs);
    for (ptrdiff_t i = 0; i < nargs; i++)
        args[i] = va_arg(vargs, emacs_value);
    va_end(vargs);

    return env->funcall(env, func, nargs, args);
}

bool em_assert_type(emacs_env *env, emacs_value predicate, emacs_value arg)
{
    bool cond = env->is_not_nil(env, em_funcall(env, predicate, 1, arg));
    if (!cond)
        em_signal_wrong_type(env, predicate, arg);
    return cond;
}

void em_signal_wrong_type(emacs_env *env, emacs_value expected, emacs_value actual)
{
    env->non_local_exit_signal(
        env, _wrong_type_argument,
        em_cons(env, expected, em_cons(env, actual, em_nil))
    );
}

char *em_get_string(emacs_env *env, emacs_value arg)
{
    ptrdiff_t size;
    env->copy_string_contents(env, arg, NULL, &size);

    char *buf = (char*) malloc(size * sizeof(char));
    env->copy_string_contents(env, arg, buf, &size);

    return buf;
}

emacs_value em_cons(emacs_env *env, emacs_value car, emacs_value cdr)
{
    return em_funcall(env, _cons, 2, car, cdr);
}

void em_defun(emacs_env *env, const char *name, emacs_value func)
{
    em_funcall(env, _defalias, 2, INTERN(name), func);
}

uint32_t em_buffer_size(emacs_env *env)
{
    return env->extract_integer(env, em_funcall(env, _buffer_size, 0));
}

bool em_buffer_contents(emacs_env *env, uint32_t offset, uint32_t nchars, char *buffer)
{
    emacs_value string = em_funcall(
        env, _buffer_substring, 2,
        env->make_integer(env, offset + 1),
        env->make_integer(env, offset + nchars + 1)
    );

    // Check that we got what we asked for
    ptrdiff_t nbytes;
    env->copy_string_contents(env, string, NULL, &nbytes);

    if (nbytes - 1 != nchars)
        return false;

    env->copy_string_contents(env, string, buffer, &nbytes);
    return true;
}

void em_provide(emacs_env *env, const char *feature)
{
    em_funcall(env, _provide, 1, INTERN(feature));
}

bool em_user_ptrp(emacs_env *env, emacs_value val)
{
    return env->is_not_nil(env, em_funcall(env, _user_ptrp, 1, val));
}
