.. index:: hush shell

.. _hush:

hush shell
==========

barebox has an integrated shell: hush. This is a simple shell which
is enough for writing simple shell scripts. Usage of the shell for
scripts should not be overstrained. Often a command written in C is
more flexible and also more robust than a complicated shell script.

hush features
-------------

variables::

	a="Hello user"
	echo $a
	Hello user

conditional execution ``if`` / ``elif`` / ``else`` / ``fi``::

	if [ ${foo} = ${bar} ]; then
		echo "foo equals bar"
	else
		echo "foo and bar differ"
	fi

``for`` loops::

	for i in a b c; do
		echo $i
	done

``while`` loops::

	while true; do
		echo "endless loop"
	done

wildcard globbing::

	ls d*
	echo ???

There is no support in hush for input/output redirection or pipes.
Some commands work around this limitation with additional arguments. for
example the :ref:`command_echo` command has the ``-a FILE`` option for appending
a file and the ``-o FILE`` option for overwriting a file. The readline
command requires a variable name as argument in which the line will be
stored.
