import sys
import traceback

from rpn2valib.tests import (
    test_codegen,
    test_emit,
    test_lexer,
    test_parser,
    test_semantics,
)

MODULES = [
    test_lexer,
    test_parser,
    test_semantics,
    test_codegen,
    test_emit,
]


def main():
    failed = 0
    for mod in MODULES:
        try:
            mod.run()
        except Exception:
            failed += 1
            print(mod.__name__+": FAILED")
            traceback.print_exc()
        else:
            print(mod.__name__+": OK")

    if failed:
        print(str(failed)+" of "+str(len(MODULES))+" test module(s) failed.")
        sys.exit(1)
    else:
        print("All "+str(len(MODULES))+" test modules passed.")


if __name__ == "__main__":
    main()
