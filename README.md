py-pointless
============

A fast and efficient read-only relocatable data structure for JSON like data, with C and Python APIs

```python
import pointless

pointless.serialize({1: 2, 3: 4}, 'file.pointless')

root = pointless.Pointless('file.pointless').GetRoot()
print(root, repr(root))

for k, v in root.items():
  print(k, '->', v)
```
