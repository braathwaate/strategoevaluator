#!/usr/bin/python -u

#NOTE: The -u option is required for unbuffered stdin/stdout.
#	If stdin/stdout are buffered, the manager program will not recieve any messages and assume that the agent has timed out.

'''
 khaos.py - A sample Stratego AI for the UCC Programming Competition 2012

 The name describes the state of this file :S

 Written in python, the slithery language 

 author Sam Moore (matches) [SZM]
 website http://matches.ucc.asn.au/stratego
 email progcomp@ucc.asn.au or matches@ucc.asn.au
 git git.ucc.asn.au/progcomp2012.git
'''

import os

from basic_python import *
from path import *

def OppositeDirection(direction):
	if direction == "UP":
		return "DOWN"
	elif direction == "DOWN":
		return "UP"
	elif direction == "LEFT":
		return "RIGHT"
	elif direction == "RIGHT":
		return "LEFT"
	else:
		assert(False)
	return "ERROR"

class Hunter(BasicAI):
	" Python based AI of DEATH "
	def __init__(self, scoresFilename=None):
		if scoresFilename == None:
			scoresFilename = "default.scores"
		BasicAI.__init__(self)
		
		scoresFile = open(scoresFilename, "r")
		self.scoreTable = []
		for i in scoresFile.readline().strip().split(' '):
			self.scoreTable.append(float(i))
		scoresFile.close()

		self.maxdepth = 1
		self.recursiveConsider = {"allies" : 5, "enemies" : 5}

		

	def PositionLegal(self, x, y, unit = None):
		if x >= 0 and x < len(self.board) and y >= 0 and y < len(self.board[x]):
			if unit == None:
				return True
			else:
				return self.board[x][y] == None or self.board[x][y].colour == oppositeColour(unit.colour)
		else:
			return False

	def BestMove(self, maxdepth = 1):

		moveList = []

		
		if maxdepth < self.maxdepth:
			#sys.stderr.write("Recurse!\n")
			considerAllies = self.recursiveConsider["allies"]
			considerEnemies = self.recursiveConsider["enemies"]
		else:
			considerAllies = len(self.units)+1
			considerEnemies = len(self.enemyUnits)+1

		for enemy in self.enemyUnits[0:considerEnemies]:
			for ally in self.units[0:considerAllies]:
				moveList.append(self.DesiredMove(ally, enemy))

		for desiredMove in moveList:
			if desiredMove[0] == "NO_MOVE" or desiredMove[2] == None:
				desiredMove[1] = -2.0

		
			

		if maxdepth > 1:
			for desiredMove in moveList:
				if desiredMove[2] == None or desiredMove[1] < 0.0:
					continue
				p = move(desiredMove[3].x, desiredMove[3].y, desiredMove[2][0], 1)
				if self.board[p[0]][p[1]] == None:
					x = desiredMove[3].x
					y = desiredMove[3].y
					result = desiredMove[0] + " OK"
					self.InterpretResult(result)
					bestRecurse = self.BestMove(maxdepth-1)
					if bestRecurse != None:
						desiredMove[1] += bestRecurse[1]# / float(max(1.0, maxdepth))
					self.board[desiredMove[3].x][desiredMove[3].y] = None
					self.board[x][y] = desiredMove[3]
					desiredMove[3].x = x
					desiredMove[3].y = y

		for desiredMove in moveList:
			if desiredMove[1] > 0.0:
				desiredMove[1] = desiredMove[1] / float(len(desiredMove[2]))
		
		if len(moveList) <= 0:
			return None
		moveList.sort(key = lambda e : e[1], reverse = True)			
		return moveList[0]
				
						
	def DesiredMove(self, ally, enemy):
		""" Determine desired move of allied piece, towards or away from enemy, with score value """
		scaleFactor = 1.0
		if ally.rank == 'F' or ally.rank == 'B':
			return ["NO_MOVE", 0, None, ally, enemy]
		
		actionScores = {"ATTACK" : 0, "RETREAT" : 0}
		if enemy.rank == '?':
			for i in range(0, len(ranks)):
				prob = self.rankProbability(enemy, ranks[i])
				if prob > 0:
					desiredAction = self.DesiredAction(ally, ranks[i])
					actionScores[desiredAction[0]] += prob* (desiredAction[1] / 2.0)
			if len(enemy.positions) <= 1 and ally.rank != '8':
				scaleFactor *= (1.0 - float(valuedRank(ally.rank)) / float(valuedRank('1')))**2.0
			elif len(enemy.positions) > 1 and ally.rank == '8':
				scaleFactor *= 0.05
			#elif len(enemy.positions) > 1:
			#	scaleFactor *= (1.0 - float(valuedRank(ally.rank)) / float(valuedRank('1')))**0.25
			#	scaleFactor = max(0.05, scaleFactor)
		else:
			desiredAction = self.DesiredAction(ally, enemy.rank)
			actionScores[desiredAction[0]] += desiredAction[1]
		

		desiredAction = sorted(actionScores.items(), key = lambda e : e[1], reverse = True)[0]
		direction = None
		#path = PathFinder().pathFind((ally.x, ally.y), (enemy.x, enemy.y), self.board)
		
		#if path != False and len(path) > 0:
		#	if desiredAction[0] == "RETREAT":
				#sys.stderr.write("Recommend retreat! "+ally.rank + " from " + enemy.rank+"\n")
		#		direction = OppositeDirection(path[0])
		#		p = move(ally.x, ally.y, direction, 1)
		#		if self.PositionLegal(p[0], p[1], ally) == False:
		#			path = None
		#		scaleFactor = 0.05 * scaleFactor
		#	else:
		#		direction = path[0]

		#	return [str(ally.x) + " " + str(ally.y) + " " + direction, desiredAction[1] * scaleFactor, path, ally, enemy]

		directions = {"RIGHT" : enemy.x - ally.x, "LEFT" : ally.x - enemy.x, "DOWN" : enemy.y - ally.y, "UP" : ally.y - enemy.y}
		if desiredAction[0] == "RETREAT":
			for key in directions.keys():
				directions[key] = -directions[key]

		while direction == None:
			d = sorted(directions.items(), key = lambda e : e[1], reverse = True)
			p = move(ally.x, ally.y, d[0][0], 1)
			if self.PositionLegal(p[0], p[1]) and (self.board[p[0]][p[1]] == None or self.board[p[0]][p[1]] == enemy):
				direction = d[0][0]
				scaleFactor *= (1.0 - float(max(d[0][1], 0.0)) / 10.0)**2.0
			else:
				del directions[d[0][0]]
				if len(directions.keys()) <= 0:
					break

		#if abs(enemy.x - ally.x) >= abs(enemy.y - ally.y):
		#	if enemy.x > ally.x:
		#		direction = "RIGHT"
		#	elif enemy.x < ally.x:
		#
		#else:
		#	if enemy.y > ally.y:
		#		direction = "DOWN"
		#	elif enemy.y < ally.y:
		#		direction = "UP"
		if direction == None:
			return ["NO_MOVE", 0, [], ally, enemy]
		return [str(ally.x) + " " + str(ally.y) + " " + direction, desiredAction[1], [direction], ally, enemy]			
			

	def DesiredAction(self, ally, enemyRank):
		if enemyRank == 'F':
			return ["ATTACK", 1.0]
		if ally.rank == '8' and enemyRank == 'B':
			return ["ATTACK", 0.9]
		if ally.rank == '1' and enemyRank == 's':
			return ["RETREAT", 0.9]
		if ally.rank == 's' and enemyRank == '1':
			return ["ATTACK", 0.6]
		if enemyRank == 'B':
			return ["RETREAT", 0.0]
		if ally.rank == enemyRank:
			return ["ATTACK", 0.1]
		if valuedRank(ally.rank) > valuedRank(enemyRank):
			return ["ATTACK", float(self.scoreTable[ranks.index(enemyRank)]) * (0.1 + 1.0/float(self.scoreTable[ranks.index(ally.rank)]))]
		else:
			return ["RETREAT", float(self.scoreTable[ranks.index(ally.rank)]) / 10.0]
		

	def MakeMove(self):
		if len(self.units) < 20:
			self.maxdepth = 1
		bestMove = self.BestMove(self.maxdepth)


		if bestMove == None:
			#sys.stderr.write("Khaos makes random move!\n")
			return BasicAI.MakeMove(self)
		
		#sys.stderr.write("Board state before move: \n")
		#self.debugPrintBoard()
		
		sys.stderr.write("Best move is \"" + bestMove[0] + "\" with score " +  str(bestMove[1]) + " as part of path " +str(bestMove[2]) + " ...\n")
		sys.stderr.write("	 Ally with rank " + bestMove[3].rank + " is targeting unit at " + str((bestMove[4].x, bestMove[4].y)) + " rank " + bestMove[4].rank + "\n")
		sys.stdout.write(bestMove[0] + "\n")


		return True



	def rankProbability(self, target, targetRank):

		if targetRank == '+' or targetRank == '?':
			return 0.0
		if target.rank == targetRank:
			return 1.0
		elif target.rank != '?':
			return 0.0

		total = 0.0
		for rank in ranks:
			if rank == '+' or rank == '?':
				continue
			elif rank == 'F' or rank == 'B':
				if target.lastMoved < 0:
					total += self.hiddenEnemies[rank]
			else:
				total += self.hiddenEnemies[rank]

		if total == 0.0:
			return 0.0
		return float(float(self.hiddenEnemies[targetRank]) / float(total))

	def InterpretResult(self, string=None):
		if BasicAI.InterpretResult(self, string) == False:
			return False


		if self.maxdepth > 1:
			if self.lastMoved != None and self.lastMoved.colour == self.colour and self.lastMoved.alive == False:
				self.units.sort(key = lambda e : valuedRank(e.rank), reverse = True)
			elif self.lastMoved != None and self.lastMoved.colour == oppositeColour(self.colour) and self.lastMoved.alive == True:
				oldRank = self.lastMoved.rank
				self.lastMoved.rank = '1'
				self.enemyUnits.sort(key = lambda e : valuedRank(e.rank), reverse = True)
				self.lastMoved.rank = oldRank
			
		
		return True			
				
		
if __name__ == "__main__":
	if len(sys.argv) > 1:
		hunter = Hunter(sys.argv[1])
	else:
		string = ""
		path = sys.argv[0].split('/')
		for i in range(0, len(path)-1):
			string += path[i] + "/"
		string += "default.scores"
		
		
		hunter = Hunter(string)
	if hunter.Setup():
		while hunter.MoveCycle():
			pass

