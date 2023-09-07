import sys

if len(sys.argv) == 0:
    sys.stderr.write("this is stderr\n")
    sys.stdout.write("this is stdout\n")
else:
    for i in sys.argv:
        sys.stderr.write(f'stderr: {i}\n')
        sys.stdout.write(f'stdout: {i}\n')
        
