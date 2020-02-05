# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# This script must be tested on bash, zsh, and dash.

_abspath () {
  python -c "import os.path; print(os.path.abspath('$@'))"
}

# Shell: bash.
if test -n "$BASH"; then
  PW_SETUP_SCRIPT_PATH=$(_abspath $BASH_SOURCE)
# Shell: zsh.
elif test -n "$ZSH_NAME"; then
  PW_SETUP_SCRIPT_PATH=$(_abspath ${(%):-%N})
# Shell: dash.
elif test ${0##*/} = dash; then
  PW_SETUP_SCRIPT_PATH=$(_abspath \
    $(lsof -p $$ -Fn0 | tail -1 | sed 's#^[^/]*##;'))
# If everything else fails, try $0. It could work.
else
  PW_SETUP_SCRIPT_PATH=$(_abspath $0)
fi

PW_ROOT=$(dirname $(dirname $PW_SETUP_SCRIPT_PATH))
export PW_ROOT

SETUP_SH="$PW_ROOT/pw_env_setup/.setup.sh"

# Try to use Python 3 if possible by default, before Python 2.
if which python3 &> /dev/null; then
  PYTHON=python3
else
  PYTHON=python
fi

# Run full bootstrap when invoked as bootstrap, or env file is missing/empty.
if \
  [ $(basename $PW_SETUP_SCRIPT_PATH) = "bootstrap.sh" ] || \
  [ ! -f $SETUP_SH ] || \
  [ ! -s $SETUP_SH ]; then
  $PYTHON $PW_ROOT/pw_env_setup/py/pw_env_setup/env_setup.py --shell-file $SETUP_SH
fi

. $SETUP_SH