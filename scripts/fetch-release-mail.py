#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Turn a barebox release announcement mail into GitHub release notes.

The announcement for a release tag is looked up on the mailing list archives
with lei(1) and printed as Markdown in the format used for the releases at
https://github.com/barebox/barebox/releases: the quoted mail text, linked to
its archive URL, followed by the migration guide link, if the tagged tree has
a migration guide for the release, and the GitHub compare link to the
previous release tag.

Archives are tried in order, so lore.barebox.org being down or lagging behind
only means that the mail is taken from the lore.kernel.org mirror instead.
As a sanity check, the mail must have been sent from the address that tagged
the release, with old addresses resolved through .mailmap.
"""

import argparse
import email
import email.policy
import email.utils
import re
import subprocess
import sys

DEFAULT_INBOXES = [
    'https://lore.barebox.org/barebox/',
    'https://lore.kernel.org/barebox/',
]
TAG_GLOB = 'v????.??.?*'


def die(*lines):
    print(*lines, sep='\n', file=sys.stderr)
    sys.exit(1)


def git(*args, check=True):
    res = subprocess.run(['git', *args], capture_output=True, text=True)
    if check and res.returncode:
        die(f"git {' '.join(args)}: {res.stderr.strip()}")
    return res.stdout.strip() if res.returncode == 0 else None


def lei_query(inbox, query):
    """Return the messages matching query in a remote public-inbox."""
    cmd = ['lei', 'q', '--no-save', '--no-import-remote', '--only', inbox,
           '-f', 'mboxrd', query]
    try:
        res = subprocess.run(cmd, stdout=subprocess.PIPE)
    except FileNotFoundError:
        die('lei not found, install the lei package (part of public-inbox)')
    if res.returncode:
        raise OSError(f'lei exited with status {res.returncode}')

    # In mboxrd, body lines that look like a separator are escaped with a
    # leading '>', so an unescaped "From " always starts a new message.
    return [email.message_from_bytes(raw, policy=email.policy.default)
            for raw in re.split(rb'^From .*\n', res.stdout, flags=re.M)[1:]]


def canonical_email(addr):
    """Return addr with old addresses mapped through .mailmap."""
    if not addr:
        return None
    return email.utils.parseaddr(git('check-mailmap', f'<{addr}>'))[1].lower()


def tagger_email(tag):
    """Return the address that created the tag, None for lightweight tags."""
    out = git('for-each-ref', '--format=%(taggeremail)', f'refs/tags/{tag}')
    return canonical_email(out.strip('<>'))


def subject_of(msg):
    return ' '.join(str(msg['Subject'] or '').split())


def sender_of(msg):
    return canonical_email(email.utils.parseaddr(str(msg['From'] or ''))[1])


def date_of(msg):
    try:
        return email.utils.parsedate_to_datetime(msg['Date']).timestamp()
    except (TypeError, ValueError):
        return float('inf')


def rank(subject, release):
    """Return how much a subject looks like the announcement, lower is better.

    Announcements have been titled "v2026.09.0", "barebox-2025.02.0",
    "v2026.08.1 and v2026.04.2" and "Stable releases v2025.09.3 and
    v2026.03.1" so far. Replies and patches mentioning the version are
    not announcements at all.
    """
    rel = r'v?' + re.escape(release)
    if re.search(r'(?i)\b(re|aw|fwd?):|\[PATCH', subject):
        return None
    if not re.search(rf'(?i)(?<![\w.]){rel}(?![\w.])', subject):
        return None
    if re.fullmatch(rf'(?i)(barebox[-. ]?)?{rel}( is out)?', subject):
        return 0
    prefix = r'(stable )?(releases?|updates?):?\s+|barebox[-. ]?'
    if re.match(rf'(?i)({prefix})?{rel}\b', subject):
        return 1
    return 2


def find_announcement(inboxes, release, sender):
    """Return (message, inbox) of the announcement mail for release.

    A phrase search for the bare version matches subjects like
    "barebox-2026.09.0", but not "v2026.09.0", which is one term for Xapian,
    so both forms are queried. Only mails from sender are accepted, if one
    is given.
    """
    seen = {}
    rejected = []
    for inbox in inboxes:
        candidates = []
        for form in (f'v{release}', release):
            try:
                msgs = lei_query(inbox, f's:"{form}"')
            except OSError as e:
                print(f'{inbox}: {e}', file=sys.stderr)
                break
            for msg in msgs:
                subject = subject_of(msg)
                seen.setdefault(subject, inbox)
                r = rank(subject, release)
                if r is None:
                    continue
                if sender and sender_of(msg) != sender:
                    rejected.append(f'\t"{subject}" from {sender_of(msg)}')
                    continue
                candidates.append((r, date_of(msg), msg))
        if candidates:
            candidates.sort(key=lambda c: c[:2])
            return candidates[0][2], inbox

    lines = [f'release mail for {release} not found.']
    if rejected:
        lines += [f'not sent by {sender}, see --sender:', *rejected]
    if seen:
        lines += ['results:', *(f'\t{s} [{i}]' for s, i in seen.items())]
    else:
        lines.append('no results at all.')
    die(*lines)


def announcement_text(msg):
    """Return the mail text before the migration notes, the shortlog or the
    signature.

    The migration notes are left out, as the release notes link the rendered
    migration guide instead. They are recognized by their heading: a blank
    line, then a line starting with "Migration notes", "Migration guide" or
    "Migration to" that either ends in a colon or is followed by an
    underline of repeated punctuation.
    """
    part = msg.get_body(preferencelist=('plain',))
    if part is None:
        die(f'no text part in <{msg["Message-ID"]}>')

    text = re.sub(r'^>(>*From )', r'\1', part.get_content(), flags=re.M)
    notes = re.search(r'\n\n(?i:migration (?:notes?|guides?|to)\b)[^\n]*'
                      r'(?::[ \t]*(?:\n|$)|\n([^\w\s])\1{5,}[ \t]*(?:\n|$))',
                      text)
    if notes:
        text = text[:notes.start()]

    lines = []
    for line in text.splitlines():
        if (re.fullmatch(r'-{2,}(\s*(?:8<|>8)\s*-*)?\s*', line) or
                re.fullmatch(r'\S.* \(\d+\):', line) or
                line.startswith('The following changes since commit')):
            break
        lines.append(line.rstrip())

    while lines and not lines[-1]:
        lines.pop()
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='generate GitHub release notes from the announcement mail')
    parser.add_argument('release', nargs='?', metavar='TAG',
                        help='release tag, with or without the leading v '
                        '(default: newest one reachable from HEAD)')
    parser.add_argument('-I', '--inbox', action='append', metavar='URL',
                        help='public-inbox to query, in order of preference. '
                        'The first one is also used for the archive link '
                        f'(default: {" ".join(DEFAULT_INBOXES)})')
    parser.add_argument('-s', '--sender', metavar='ADDR',
                        help='address the announcement must have been sent '
                        'from (default: the one that created the tag)')
    parser.add_argument('--no-sender-check', action='store_true',
                        help='accept the announcement from any sender')
    parser.add_argument('--repo', default='barebox/barebox',
                        metavar='OWNER/NAME',
                        help='GitHub repository for the compare link')
    parser.add_argument('-o', '--output', type=argparse.FileType('w'),
                        default=sys.stdout, metavar='FILE',
                        help='write notes to FILE')
    args = parser.parse_args()

    inboxes = args.inbox or DEFAULT_INBOXES
    tag = args.release or git('describe', '--abbrev=0', '--match', TAG_GLOB)
    if not tag.startswith('v'):
        tag = 'v' + tag
    release = tag[1:]
    is_tag = git('rev-parse', '-q', '--verify', f'{tag}^{{commit}}',
                 check=False) is not None

    sender = None
    if args.no_sender_check:
        pass
    elif args.sender:
        sender = canonical_email(args.sender)
    elif not is_tag:
        print(f'{tag} is not a tag in this repository, skipping the '
              'sender check', file=sys.stderr)
    elif not (sender := tagger_email(tag)):
        print(f'{tag} is not an annotated tag, skipping the sender check',
              file=sys.stderr)
    if not is_tag:
        print(f'{tag} is not a tag in this repository, skipping the '
              'migration guide and changelog links', file=sys.stderr)

    msg, inbox = find_announcement(inboxes, release, sender)
    msgid = str(msg['Message-ID']).strip().strip('<>')
    name, addr = email.utils.parseaddr(str(msg['From']))
    print(f'<{msgid}> "{subject_of(msg)}" found on {inbox}', file=sys.stderr)

    out = [f'[On {str(msg["Date"]).strip()}, {name or addr} wrote]'
           f'({inboxes[0].rstrip("/")}/{msgid}/)']
    out += [f'> {line}'.rstrip() for line in announcement_text(msg)]
    out.append('')

    # The guide is usually still named migration-master.rst at the tag and
    # renamed right after it, so either name counts
    guides = (f'Documentation/migration-guides/migration-{n}.rst'
              for n in (release, 'master'))
    if any(git('cat-file', '-e', f'{tag}:{g}', check=False) is not None
           for g in guides):
        out.append('**Migration Guide**: https://barebox.org/doc/latest/'
                   f'migration-guides/migration-{release}.html')

    prev = git('describe', '--abbrev=0', '--tags', '--match', TAG_GLOB,
               f'{tag}~1', check=False)
    if prev:
        out.append(f'**Full Changelog**: https://github.com/{args.repo}/'
                   f'compare/{prev}...{tag}')

    print(*out, sep='\n', file=args.output)


if __name__ == '__main__':
    main()
