#!/usr/bin/python

from __future__ import print_function

import errno
import os
import re
import sys
import hashlib

from collections import defaultdict
from pprint import pprint

# TODO: handle commands with the same name in multiple files
# TODO: handle #ifdefs

HELP_START = re.compile(r"""^BAREBOX_CMD_HELP_START\s*\((\w+)\)?\s*$""")
HELP_TEXT  = re.compile(r"""^BAREBOX_CMD_HELP_TEXT\s*\("(.*?)"\)?\s*$""")
HELP_OPT   = re.compile(r"""^BAREBOX_CMD_HELP_OPT\s*\("(.+?)",\s*"(.+?)"\)?\s*$""")
HELP_END   = re.compile(r"""^BAREBOX_CMD_HELP_END\s*$""")

CMD_START  = re.compile(r"""^BAREBOX_CMD_START\s*\((.+)\)\s*$""")
CMD_FUNC   = re.compile(r"""^\s*\.cmd\s*=\s*(.+?),\s*$""")
CMD_DESC   = re.compile(r"""^\s*BAREBOX_CMD_DESC\s*\("(.*?)"\)?\s*$""")
CMD_OPTS   = re.compile(r"""^\s*BAREBOX_CMD_OPTS\s*\("(.*?)"\)?\s*$""")
CMD_GROUP  = re.compile(r"""^\s*BAREBOX_CMD_GROUP\s*\((.+)\)\s*$""")
CMD_END    = re.compile(r"""^BAREBOX_CMD_END\s*$""")

CONT = re.compile(r"""\s*"(.*?)"\s*\)?\s*$""")

CMDS = {}

def string_escape(s):
  # This used to do s.decode("string_escape") which isn't available on Python 3.
  # Actually we only need to drop '\t' and '\n', so do this here.
  return s.replace(r'\t', '').replace(r'\n', '')

def parse_c(name):
  cmd = None
  last = None
  for line in open(name, 'r'):
    x = HELP_START.match(line)
    if x:
      cmd = CMDS.setdefault(x.group(1), defaultdict(list))
      cmd.setdefault("files", set()).add(name)
      continue
    x = CMD_START.match(line)
    if x:
      cmd = CMDS.setdefault(x.group(1), defaultdict(list))
      cmd.setdefault("files", set()).add(name)
      continue
    if cmd is None:
      continue
    x = HELP_TEXT.match(line)
    if x:
      if 'h_opts' not in cmd:
        last = cmd['h_pre']
      else:
        last = cmd['h_post']
      last.append(string_escape(x.group(1)).strip())
      continue
    x = HELP_OPT.match(line)
    if x:
      last = cmd['h_opts']
      last.append([
        string_escape(x.group(1)),
        string_escape(x.group(2)),
      ])
      continue
    x = CMD_FUNC.match(line)
    if x:
      last = cmd['c_func']
      last.append(x.group(1))
      continue
    x = CMD_DESC.match(line)
    if x:
      last = cmd['c_desc']
      last.append(string_escape(x.group(1)))
      continue
    x = CMD_OPTS.match(line)
    if x:
      last = cmd['c_opts']
      last.append(string_escape(x.group(1)))
      continue
    x = CMD_GROUP.match(line)
    if x:
      last = cmd['c_group']
      last.append(x.group(1).split('_')[-1].lower())
      continue
    x = CONT.match(line)
    if x:
      if last is None:
        raise Exception("Parse error in %s: %r" % (name, line))
      if isinstance(last[-1], str):
        last[-1] += string_escape(x.group(1))
      elif isinstance(last[-1], list):
        last[-1][1] += string_escape(x.group(1))
      continue
    x = HELP_END.match(line)
    if x:
      cmd = last = None
    x = CMD_END.match(line)
    if x:
      cmd = last = None

def gen_rst(name, cmd):
  out = []
  out.append('.. index:: %s (command)' % name)
  out.append('')
  out.append('.. _command_%s:' % name)
  out.append('')
  if 'c_desc' in cmd:
    out.append("%s - %s" % (name, ''.join(cmd['c_desc']).strip()))
  else:
    out.append("%s" % (name,))
  out.append('='*len(out[-1]))
  out.append('')
  if 'c_opts' in cmd:
    out.append('Usage')
    out.append('^'*len(out[-1]))
    out.append('``%s %s``' % (name, ''.join(cmd['c_opts']).strip()))
    out.append('')
  if 'h_pre' in cmd:
    pre = cmd['h_pre']
    if pre and pre[-1] == "Options:":
      del pre[-1]
    if pre and pre[-1] == "":
      del pre[-1]
    if pre:
      out.append('Synopsis')
      out.append('^'*len(out[-1]))
      out.append('\n'.join(cmd['h_pre']).strip())
      out.append('')
  if 'h_opts' in cmd:
    out.append('Options')
    out.append('^'*len(out[-1]))
    for o, d in cmd['h_opts']:
      o = o.strip()
      d = d.strip()
      if o:
        out.append('%s\n %s' % (o, d))
      else:
        out.append(' %s' % (d,))
    out.append('')
  if 'h_post' in cmd:
    post = cmd['h_post']
    if post and post[0] == "":
      del post[0]
    if post:
      out.append('Description')
      out.append('^'*len(out[-1]))
      out.append('\n'.join(cmd['h_post']).strip())
      out.append('')
  out.append('.. generated from: %s' % ', '.join(cmd['files']))
  if 'c_func' in cmd:
    out.append('.. command function: %s' % ', '.join(cmd['c_func']))
  return '\n'.join(out)

for root, dirs, files in os.walk(sys.argv[1]):
  for name in files:
    if name.endswith('.c'):
      source = os.path.join(root, name)
      parse_c(source)

for name in CMDS.keys():
  CMDS[name] = dict(CMDS[name])

for name, cmd in CMDS.items():
  #pprint({name: cmd})
  rst = gen_rst(name, cmd)
  group = cmd.get('c_group')
  if group is None:
    print("gen_commands: warning: using default group 'misc' for command '%s'" % name, file=sys.stderr)
    group = ['misc']
  subdir = os.path.join(sys.argv[2], group[0])
  try:
    os.makedirs(subdir)
  except OSError as e:
    if e.errno == errno.EEXIST and os.path.isdir(subdir):
      pass
    else:
      raise
  target = os.path.join(subdir, name+'.rst')

  # Only write the new rst if it differs from the old one. Wroto
  hash_old = hashlib.sha1()
  try:
    f = open(target, 'rb')
    hash_old.update(f.read())
  except:
    pass
  hash_new = hashlib.sha1()
  hash_new.update(rst.encode('utf-8'))
  if hash_old.hexdigest() == hash_new.hexdigest():
    continue

  open(target, 'w').write(rst)
