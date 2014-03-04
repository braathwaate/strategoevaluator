
import sys
import random


class PathFinder:
	def __init__(self):
		self.visited = []

		pass

	def pathFind(self, start, end, board):
		if start[0] == end[0] and start[1] == end[1]:
			#sys.stderr.write("Got to destination!\n")
			return []

		if self.visited.count(start) > 0:
			#sys.stderr.write("Back track!!\n")
			return False
		if start[0] < 0 or start[0] >= len(board) or start[1] < 0 or start[1] >= len(board[start[0]]):
			#sys.stderr.write("Out of bounds!\n")
			return False
		if len(self.visited) > 0 and board[start[0]][start[1]] != None:
			#sys.stderr.write("Full position!\n")
			return False


		
		self.visited.append(start)
		left = (start[0]-1, start[1])
		right = (start[0]+1, start[1])
		up = (start[0], start[1]-1)
		down = (start[0], start[1]+1)
		choices = [left, right, up, down]
		choices.sort(key = lambda e : (e[0] - end[0])**2.0 + (e[1] - end[1])**2.0 )
		options = []
		for point in choices:
			option = [point, self.pathFind(point,end,board)]
			if option[1] != False:
				options.append(option)	

		options.sort(key = lambda e : len(e[1]))
		if len(options) == 0:
			#sys.stderr.write("NO options!\n")
			return False
		else:
			if options[0][0] == left:
				options[0][1].insert(0,"LEFT")
			elif options[0][0] == right:
				options[0][1].insert(0,"RIGHT")
			elif options[0][0] == up:
				options[0][1].insert(0,"UP")
			elif options[0][0] == down:
				options[0][1].insert(0,"DOWN")
		#sys.stderr.write("PathFind got path " + str(options[0]) + "\n")
		return options[0][1]
		
