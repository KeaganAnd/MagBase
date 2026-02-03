#!/bin/bash
set -e

cd /home/keagan/Code/toy-db/build

# Clean
rm -f ../test.mab

# Rebuild
make -j4 > /dev/null 2>&1

# Test workflow
echo "=== Creating database ==="
./magbase -p test << EOF
y
EOF

echo ""
echo "=== Creating table ==="
./magbase -create-table test.mab users 2 name:text:0 last:text:1

echo ""
echo "=== Listing tables ==="
./magbase -list-tables test.mab

echo ""
echo "=== Inserting record ==="
./magbase -insert-record test 1 "Keagan" "Anderson"
