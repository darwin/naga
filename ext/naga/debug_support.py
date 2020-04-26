import os

if os.environ.get('NAGA_WAIT_FOR_DEBUGGER') is not None:
    input("Waiting for debugger, please hit ENTER when ready...")