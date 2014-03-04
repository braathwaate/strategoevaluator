#!/usr/bin/python -u

#NOTE: The -u option is required for unbuffered stdin/stdout.
#	If stdin/stdout are buffered, the manager program will not recieve any messages and assume that the agent has timed out.

'''
 asmodeus.py - A sample Stratego AI for the UCC Programming Competition 2012

 Written in python, the slithery language 

 author Sam Moore (matches) [SZM]
 website http://matches.ucc.asn.au/stratego
 email progcomp@ucc.asn.au or matches@ucc.asn.au
 git git.ucc.asn.au/progcomp2012.git
'''

from basic_python import *
from path import *



class Asmodeus(BasicAI):
	" A slightly more advanced python based AI who calculates the optimum score for each move "
	def __init__(self):
		#sys.stderr.write("Asmodeus initialised...\n")
		BasicAI.__init__(self)
		self.riskScores = {'1' : 0.01 , '2' : 0.05 , '3' : 0.15 , '4' : 0.2, '5' : 0.2, '6' : 0.25, '7' : 0.25, '8' : 0.01 , '9' : 0.4, 's' : 0.01}
		self.bombScores = {'1' : 0.0 , '2' : 0.0 , '3' : 0.05 , '4' : 0.1, '5' : 0.3, '6' : 0.4, '7' : 0.5, '8' : 1 , '9' : 0.6, 's' : 0.1}
		self.flagScores = {'1' : 1.0 , '2' : 1.0 , '3' : 1.0 , '4' : 1.0, '5' : 1.0, '6' : 1.0, '7' : 1, '8' : 1.0 , '9' : 1.0, 's' : 1.0}
		self.suicideScores = {'1' : 0.0 , '2' : 0.0 , '3' : 0.0, '4' : 0.0, '5' : 0.0, '6' : 0.05, '7' : 0.1, '8' : 0.0 , '9' : 0.0, 's' : 0.0}
		self.killScores = {'1' : 1.0 , '2' : 0.9 , '3' : 0.8 , '4' : 0.5, '5' : 0.5, '6' : 0.5, '7' : 0.4, '8' : 0.9 , '9' : 0.6, 's' : 0.9}	

	def MakeMove(self):
		#sys.stderr.write("Asmodeus MakingMove...\n")
		"Over-rides the default BasicAI.MakeMove function"

		moveList = []

		for unit in self.units:
			if unit.mobile() == False:
				continue

			for enemy in self.enemyUnits:
				if enemy == unit:
					continue
				path = PathFinder().pathFind((unit.x, unit.y), (enemy.x, enemy.y), self.board)

				#sys.stderr.write("Computed path: " + str(path) + "\n")
				if path == False or len(path) <= 0:
					continue
				score = self.CalculateScore(unit, enemy)

				score = float(score / float(len(path) + 1))
				moveList.append([unit, path, enemy, score])


		
		if len(moveList) <= 0:
			#sys.stderr.write("NO Moves!\n")
			return BasicAI.MakeMove(self)

		moveList.sort(key = lambda e : e[len(e)-1], reverse=True)

		#sys.stderr.write("Chosen move is: " + str(moveList[0][0].x) + " " + str(moveList[0][0].y) + " " + moveList[0][1][0] + " (targeting enemy with rank " + moveList[0][2].rank + " at position " + str(moveList[0][2].x) + " " + str(moveList[0][2].y) + " (my rank " + moveList[0][0].rank+")\n")
		print str(moveList[0][0].x) + " " + str(moveList[0][0].y) + " " + moveList[0][1][0]
		return True	

	def CalculateScore(self, attacker, defender):
		if defender.rank == '?':
			return self.riskScores[attacker.rank]
		elif defender.rank == 'B':
			return self.bombScores[attacker.rank]
		elif defender.rank == 'F':
			return self.flagScores[attacker.rank]
		elif defender.valuedRank() < attacker.valuedRank() or defender.rank == '1' and attacker.rank == 's':
			return self.killScores[defender.rank]
		else:
			return self.suicideScores[attacker.rank]
		
if __name__ == "__main__":
	asmodeus = Asmodeus()
	if asmodeus.Setup():
		while asmodeus.MoveCycle():
			pass

