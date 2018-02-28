#!/bin/bash

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

MONACOIND=${MONACOIND:-$SRCDIR/monacoind}
MONACOINCLI=${MONACOINCLI:-$SRCDIR/monacoin-cli}
MONACOINTX=${MONACOINTX:-$SRCDIR/monacoin-tx}
MONACOINQT=${MONACOINQT:-$SRCDIR/qt/monacoin-qt}

[ ! -x $MONACOIND ] && echo "$MONACOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
MONAVER=($($MONACOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for bitcoind if --version-string is not set,
# but has different outcomes for bitcoin-qt and bitcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$MONACOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $MONACOIND $MONACOINCLI $MONACOINTX $MONACOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${MONAVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${MONAVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m