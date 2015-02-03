# -*- coding: utf-8 -*-

import serial, os, datetime, sys, zlib
from struct import *
import collections
import xlwt

infile = 'd:\eeprom.bin'
out_excel = 'd:\eeprom.xls'

########################################################################################################################

class PartitionHeader :
	STEEPROM_HEAD_SIZE = 252
	def __init__( self, data ):
		self.data = data

	# разбирает байты из self.data на поля -- параметры раздела
	def parse( self ):
		( self.version, self.begin, self.end, self.namelen, self.sectors_n,\
			 self.sect_len, self.crc ) = unpack( '>IIIBII 227x i', (self.data) )
		return self.crc == zlib.crc32( self.data[:-4] )

	# печатает свои поля относящиеся к параметрам раздела
	def display( self ):
		print 'Version: ', self.version
		print 'Begin: ', self.begin
		print 'End: ', self.end
		print 'Namelen: ', self.namelen
		print 'Sectors number: ', self.sectors_n
		print 'Sectors len: ', self.sect_len

class PartitionHashTable :
	HASH_ENTRIES_N = 256
	def __init__( self, data ):
		self.data = data

	def parse( self ):
		hashtable_data = unpack( '>%sI i' % self.HASH_ENTRIES_N, self.data )
		self.pointers = hashtable_data[ 0:self.HASH_ENTRIES_N ]
		self.crc = hashtable_data[-1]
		return self.crc == zlib.crc32( self.data[:-4] )

	def hash( self, str ):
		hash = 5381
		c = [ ord(x) for x in str ]
		mod32 = int( '100000000', 16 )
		mod8 = int( '100', 16 )
		for x in c :
			if( x == 0 ):
				break
			hash = (((hash * 32 % mod32) + hash) % mod32 + x) % mod32
		return hash % mod8

	def sect_address( self, str ):
		hash = self.hash( str )
		if( hash >= 256 ):
			raise NameError('Hash overflow')
		return self.pointers[ hash ]

class Sector :
	STEEPROM_SECTOR_DATA_SIZE = 16
	def __init__( self, data, namelen, datalen ):
		self.data = data
		self.namelen = namelen
		self.datalen = datalen

	def parse( self ):
		sector_index = 0
		self.name, self.sect_status, self.data_crc, self.next_hash_sect,\
		self.prev_hash_sect, self.record_len, self.sector_data_len, self.next_sect \
			= unpack( '>%ss IIII HH I' % self.namelen, self.data[sector_index:56] )
		sector_index = sector_index + 56
		self.sect_data = unpack( '>%sB' % self.STEEPROM_SECTOR_DATA_SIZE,\
					self.data[sector_index:sector_index+self.STEEPROM_SECTOR_DATA_SIZE])
		sector_index = sector_index + self.STEEPROM_SECTOR_DATA_SIZE
		self.crc = unpack( '>i', self.data[sector_index:sector_index+4])[0]

		return zlib.crc32( self.data[:-4] ) == self.crc

class Partition :
	STEEPROM_HEAD_SIZE = 252
	STEEPROM_HASHTABLE_SIZE = 1028
	def __init__( self, data ):
		self.data = data

		eeprom_index = 0
		self.header = PartitionHeader( self.data[0:self.STEEPROM_HEAD_SIZE] )
		eeprom_index = eeprom_index + self.STEEPROM_HEAD_SIZE
		if not self.header.parse() :
			raise NameError( 'Header parse failed' )
		self.header.display()
		self.hashtable = PartitionHashTable( self.data[eeprom_index:eeprom_index + self.STEEPROM_HASHTABLE_SIZE] )
		eeprom_index = eeprom_index + self.STEEPROM_HASHTABLE_SIZE
		if not self.hashtable.parse() :
			raise 'HashTable parse failed'
			
		self.sectors = []
		actual_params = 0
		for i in range( self.header.sectors_n ):
			sect = Sector( self.data[ eeprom_index:eeprom_index + self.header.sect_len ], \
											self.header.namelen, self.header.sect_len - self.header.namelen - 4 - 24 )
			eeprom_index = eeprom_index + self.header.sect_len
			if( sect.parse() ):
				self.sectors.append( sect )
				if sect.sect_status == 16 :
					print 'Sector name: ', sect.name
					actual_params = actual_params + 1
			else:
				print 'Failed to parse sector num ', i

		print 'Params number: ', actual_params

########################################################################################################################
try: 
	in_file = open( infile, 'rb' )
	eeprom = in_file.read()
	print 'Len eeprom: ', len(eeprom)
	in_file.close()

	part1 = Partition( eeprom[:65536] )
	part2 = Partition( eeprom[65536:] )

	font0 = xlwt.Font()
	font0.name = 'Consolas'
	font0.colour_index = 0
	font0.bold = False
	style_norm = xlwt.XFStyle()
	style_norm.font = font0

	font1 = xlwt.Font()
	font1.name = 'Times New Roman'
	font1.colour_index = 0
	font1.bold = True
	style_header = xlwt.XFStyle()
	style_header.font = font1

	wb = xlwt.Workbook()
	ws_part1 = wb.add_sheet('Partition 1 Header')

	ws_part1.write(0, 0, 'Version', style_header )
	ws_part1.write(0, 1, part1.header.version, style_header )
	ws_part1.write(1, 0, 'Begin', style_header )
	ws_part1.write(1, 1, part1.header.begin, style_header )
	ws_part1.write(2, 0, 'End', style_header )
	ws_part1.write(2, 1, part1.header.end, style_header )
	ws_part1.write(3, 0, 'Namelen', style_header )
	ws_part1.write(3, 1, part1.header.namelen, style_header )
	ws_part1.write(4, 0, 'Sectors number', style_header )
	ws_part1.write(4, 1, part1.header.sectors_n, style_header )
	ws_part1.write(5, 0, 'Sector len', style_header )
	ws_part1.write(5, 1, part1.header.sect_len, style_header )

	ws_hash1 = wb.add_sheet('Partition 1 Hash')
	line = 0
	hash = 0
	mod32 = int( '100000000', 16 )
	for pt in part1.hashtable.pointers :
		
		if pt != mod32-1 :
			ws_hash1.write( line, 0, hash )
			ws_hash1.write( line, 1, pt )
			sect = part1.sectors[pt]
			ws_hash1.write( line, 2, sect.name )
			if sect.next_hash_sect != sect.prev_hash_sect :
				col = 3
				while True :
					sect = part1.sectors[ sect.next_hash_sect ]
					ws_hash1.write( line, col, sect.name )
					col = col + 1
					if( sect.next_hash_sect == pt ):
						break

			line = line + 1
		# else:
			# ws_hash1.write( line, 0, 'FREE' )
		hash = hash + 1


	ws_part2 = wb.add_sheet('Partition 2 Header')

	ws_part2.write(0, 0, 'Version', style_header )
	ws_part2.write(0, 1, part2.header.version, style_header )
	ws_part2.write(1, 0, 'Begin', style_header )
	ws_part2.write(1, 1, part2.header.begin, style_header )
	ws_part2.write(2, 0, 'End', style_header )
	ws_part2.write(2, 1, part2.header.end, style_header )
	ws_part2.write(3, 0, 'Namelen', style_header )
	ws_part2.write(3, 1, part2.header.namelen, style_header )
	ws_part2.write(4, 0, 'Sectors number', style_header )
	ws_part2.write(4, 1, part2.header.sectors_n, style_header )
	ws_part2.write(5, 0, 'Sector len', style_header )
	ws_part2.write(5, 1, part2.header.sect_len, style_header )

	wb.save( out_excel )

finally:
	pass
