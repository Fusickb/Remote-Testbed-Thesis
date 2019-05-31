import csv, sys	
from decimal import *
from experimenteditor.models import SPNPGNEntry
with open('cs1939_012012-2.csv',newline='') as csvf:
	csvr = csv.reader(csvf, delimiter='|', quotechar='~')
	first = True
	pgn = None
	spn = None
	spn_length = None
	for row in csvr:
		if first:
			first = False
			continue
		pgn = None
		spn = None
		spn_length = None
		try:
			if row[0]:
				pgn = int(row[0])
			if row[6]:
				spn = int(row[6])
			if row[5]:
				spn_length = int(row[5])
			_, created = SPNPGNEntry.objects.get_or_create(pgn=pgn, spn=spn, pgn_length=row[2], name=row[7], spn_length=spn_length, description=row[8], pgl=row[1], position=row[4], transmissionrate_ms=row[3], units=row[10], offset=row[9])	 
		except ValueError:
			print(' '.join([row[0],row[6],row[5],row[2]]))
			sys.exit(1)
