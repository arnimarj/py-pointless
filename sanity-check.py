import pointless


value = {
	'foo': 10,
	10: 'foo',
	(): set(),
}

pointless.serialize(value, 'value.map')
print(pointless.Pointless('value.map').GetRoot())
