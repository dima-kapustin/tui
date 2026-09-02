import sys

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP')
j = data.find(b'\x1b\\', i)
img = data[i:j + 2]
defs = []
k = 0
while k < len(img):
    if img[k:k + 1] == b'#':
        m = k + 1
        n = ''
        while m < len(img) and chr(img[m]).isdigit():
            n += chr(img[m])
            m += 1
        if m < len(img) and img[m:m + 1] == b';':
            defs.append(int(n))
        k = m
    else:
        k += 1
print("palette definitions:", defs)
print("unique:", len(set(defs)), "total:", len(defs))
