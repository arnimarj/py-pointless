#!/usr/bin/python

def my_print(depth, s):
	print '%s%s' % ('   ' * depth, s)

def strongly_connected_components_recursive(G, source):
	def visit(v, cnt, depth):
		my_print(depth, 'pointless_cycle_marker_visit(v = %u, count = %u):' % (v, cnt))
		root[v] = cnt
		my_print(depth, ' root[%u] = %u' % (v, cnt))
		visited[v] = cnt
		my_print(depth, ' visited[%u] = %u' % (v, cnt))
		my_print(depth, ' count = %u + 1' % (cnt,))
		cnt += 1
		stack.append(v)
		my_print(depth, ' stack.append(%u)' % (v,))

		for w in G[v]:
			my_print(depth + 1, 'process_child(w = %i, count = %i)' % (w, cnt))

			if w not in visited:
				my_print(depth + 1, ' w is not in visited')
				my_print(depth + 1, '  visit(w, %i)' % (cnt,))
				visit(w, cnt, depth + 2)
			else:
				my_print(depth + 1, ' w is in visited')
				pass

			if w not in component:
				my_print(depth + 1, ' w is not in component')

				if root[w] < root[v]:
					my_print(depth + 1, 'root[v] = min(%u, %u)' % (root[v], root[w]))

				root[v] = min(root[v],root[w])
			else:
				my_print(depth + 1, ' w is in component')
				pass

		my_print(depth, ' if root[%u] (%u) == visited[%u] (%u)' % (v, root[v], v, visited[v]))

		if root[v] == visited[v]:
			my_print(depth, '  component[%u] = root[%u] (%u)' % (v, v, root[v]))
			component[v] = root[v]
			tmpc = [v]

			my_print(depth, '  while stack[-1] != %u' % (v,))

			while stack[-1] != v:
				my_print(depth, '  w = stack.pop() (%u)' % (stack[-1],))
				w = stack.pop()
				my_print(depth, '  component[%u] = root[%u] (%u)' % (w, v, root[v]))
				component[w] = root[v]
				tmpc.append(w)

			my_print(depth, '  len(stack) == %u' % (len(stack),))
			my_print(depth, '  stack.pop()')
			stack.pop()
			scc.append(tmpc)

	scc = []
	visited = {}
	component = {}
	root = {}
	cnt = 0
	stack = []

	visit(source, cnt, 0)

	scc.sort(key = len)
	return scc

if __name__ == '__main__':
	# G = {4: [2, 3], 2: [], 3: [4]}
	G = {
		1: [7],
		7: [3, 4],
		6: [],
		5: [],
		3: [],
		4: []
	}

	strongly_connected_components_recursive(G, 1)
