#!/bin/bash

set -xe

mkdir ./wheelhouse

basename="$(dirname "$0")"
git clone https://github.com/pyenv/pyenv.git "$basename/.pyenv"

mkdir -p "$basename/wheelhouse/"

export PYENV_ROOT="$basename/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"

eval "$(pyenv init --path)"

declare -a pythons=("3.6.14" "3.7.11" "3.8.11" "3.9.6" "3.10.0rc1")

pyenv install --list

for py in "${pythons[@]}"
do
	echo "installing $py"
	pyenv install -s "$py"

	#pyenv shell "$py"
	#python -m virtualenv -q "venv-$py"

	"$basename/.pyenv/versions/$py/bin/pip" install -U virtualenv pip > /dev/null
	"$basename/.pyenv/versions/$py/bin/python3" -m virtualenv -q "venv-$py"

	echo " ..version installed" "$(./venv-"$py"/bin/python --version)"

	"./venv-$py/bin/pip" install -U pip setuptools wheel > /dev/null
	rm -rf ./dist/ ./build/
	"./venv-$py/bin/python" setup.py clean > /dev/null
	"./venv-$py/bin/python" setup.py bdist_wheel > /dev/null
	cp dist/*.whl ./wheelhouse/
done

ls -l ./wheelhouse
