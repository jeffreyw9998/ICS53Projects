import sys

for line in sys.stdin:
    if 'q' == line.rstrip():
        break
    sys.stderr.write(f'err: {line}');
    
