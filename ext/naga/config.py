# This module can be used to configure naga behaviour
# You want to tweak globals here before importing any naga modules/packages

import os

naga_auto_init = True

naga_wait_for_debugger = False
if os.environ.get('NAGA_WAIT_FOR_DEBUGGER') is not None:
    naga_wait_for_debugger = True
