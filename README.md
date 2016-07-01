py-pointless
============

A fast and efficient read-only relocatable data structure for JSON like data, with C and Python APIs

```python
import pointless

pointless.serialize({1:2,3:4}, 'file.pointless')

object = pointless.Pointless('file.pointless').GetRoot()
print object, type(object)

for k, v in object.iteritems():
  print k, '->', v
```
