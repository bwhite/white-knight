import InputDetection
import sys
import time
import math
def ReadConfig(file_name):
	print 'Opening config file', file_name
	config_file = open(file_name, mode='r')
	config_list = []
	while 1:
		current_line = config_file.readline()
		if len(current_line) == 0:
			break
		comment_pos = current_line.find('#')#Find first comment position
		if comment_pos == -1:
			comment_pos = len(current_line)
		elif comment_pos == 0:
			continue
		if len(current_line[0:comment_pos].split()) != 2:
			#print 'Config - 2 strings expected (white-space separated)',current_line[0:current_line.find('#')].split()
			continue
		if current_line[0:comment_pos].split()[0] == '@':
			last_slash_index = file_name.rfind('/')
			if last_slash_index == -1:
				last_slash_index = 0
			else:
				last_slash_index = last_slash_index + 1
			config_list = config_list + ReadConfig(file_name[0:last_slash_index] + current_line[0:comment_pos].split()[1])
		config_element = InputDetection.Dim1VecStr()
		#print 'Config - Name:', current_line[0:comment_pos].split()[0], 'Value:',current_line[0:comment_pos].split()[1]
		config_element.push_back(current_line[0:comment_pos].split()[0])
		config_element.push_back(current_line[0:comment_pos].split()[1])
		config_list.append(config_element)
	config_file.close()
	return config_list

#Note that this has a different syntax than the c++ version does!
def find_value(config_list, value_name):
	found_match = False
	for config_entry in config_list:
		if config_entry[0] == value_name:
			print 'CONFIG:',value_name,config_entry[1]
			return config_entry[1]
	return None

def PushElement(x):
	global config_list
	config_list.push_back(x)

#Takes an object and returns true if it passes all of the requirements for an object to be considered in matching
def ViableObject(x):
	area = x.value('area')
	areaThreshold = 40 #TODO Constants need to be in a config file!
	if int(area) < areaThreshold:
		#print 'Object being dropped because its area is too low:',area,'<',areaThreshold
		return 0
	return 1
	#TODO Add more things to drop out objects!

#This class stored the information needed related to a particular match between objects
class ObjectPairing(object):
	def __init__(self,current,history,energy):
		self.current = x
		self.history = y
		self.energy = energy
		self.dropped = False
		
#Class for an object to be tracked (intended to be the base for both current, and history objects)
class SurveillanceObject(object):
	def __get_id():
		object_counter = 1
		while 1:
			cur_val = object_counter
			object_counter = object_counter + 1
			yield cur_val
	__id_provider = __get_id()
	def __init__(self,feature_list):
		#print feature_list
		self.__features = feature_list
		self.object_id = 0
		self.frames_matched = 0
		self.frames_since_match = 0
		self.matched = False
		self.dropped = False
		self.velocity_x = 0
		self.velocity_y = 0
		for attribute in self.__features:
			if len(attribute) != 2:
				print 'Error: Feature is of size',len(self.attribute)
				exit()#TODO Throw error
	#This takes in a list of name/value pairs (also lists), and returns the value given a name(case sensitive) or NULL if not found
	def dump_features(self):
		print self.__features
	def get_id(self):
		self.object_id = self.__id_provider.next()
	def reset_matched(self):
		self.matched = False
	def value(self,name):
		for attribute in self.__features:
			if attribute[0] == name:
				return attribute[1]
		print 'value: Value',name,'not found in object, this should be an exception!'
		self.dump_features()
		exit()
	def valuef(self,name):
		for attribute in self.__features:
			if attribute[0] == name:
				return float(attribute[1])
		print 'valuef: Value',name,'not found in object, this should be an exception!'
		self.dump_features()
		exit()
	def valuei(self,name):
		for attribute in self.__features:
			if attribute[0] == name:
				return int(attribute[1])
		print 'valuei: Value',name,'not found in object, this should be an exception!'
		self.dump_features()
		exit()
	def value_set(self,name,val):
		for attribute in self.__features:
			if attribute[0] == name:
				attribute[1] = str(val)
				return
		print 'value_set: Value',name,'not found in object, this should be an exception!'
		self.dump_features()
		exit()
	def box_data(self):
		return [self.valuei('tl_x'),self.valuei('tl_y'),self.valuei('br_x'),self.valuei('br_y'),self.object_id]
	def log_data(self):
		return [self.valuei('tl_x'),self.valuei('tl_y'),self.valuei('br_x'),self.valuei('br_y'),self.object_id,self.valuef('person_probability'),self.valuef('car_probability')]


#Populated the global match_list with the match energy of each of the history_objects which are then placed in ascending match energy order into the list
def CalculateMatchEnergy(current, history):
	#Weights used for energy calculation (TODO should be located in config file)
	area_weight = 50
	pos_weight = 5
	mean_gray_weight = 1
	height_weight = 1
	width_weight = 1
	
	#Check to see if these objects pass some heuristics (area, spatial location, etc) that can quickly tell us if we won't consider them the same and can be neglected for matching
	#TODO Check to see how far the objects are from eachother, if too far then quit early
	
	#Calculate the energy of the 2 objects
	#Percent area changed
	
	area_delta = float(abs(current.valuef('area') - history.valuef('area')))/history.valuef('area')
	#Centroid Position Difference
	pos_delta = math.sqrt((current.valuef('cent_x') - (history.valuef('cent_x') + history.velocity_x))**2+(current.valuef('cent_y') - (history.valuef('cent_y') + history.velocity_y))**2)
	#Average Gray Value Difference
	mean_gray_delta = abs(current.valuef('mean_gray') - history.valuef('mean_gray'))
	#Hight Difference
	height_delta = abs(current.valuef('height') - history.valuef('height'))
	#Width Difference
	width_delta = abs(current.valuef('width') - history.valuef('width'))	
	
	energy = area_delta*area_weight+pos_delta*pos_weight+mean_gray_delta*mean_gray_weight+height_delta*height_weight+width_delta*width_weight
	print "ID:",history.object_id,"Obj1:(",current.valuef("cent_x"),",",current.valuef("cent_y"),")",current.valuef("area"),"Obj2:(",history.valuef("cent_x"),",",history.valuef("cent_y"),")",history.valuef("area")
	print 'Match Deltas - Area',area_delta,'Position',pos_delta,'Mean Gray Value',mean_gray_delta,'Height',height_delta,'Width',width_delta,'Energy',energy

	global match_list
	match_list.append(ObjectPairing(current,history,energy))
if len(sys.argv) < 2:
	print 'The second command line option must be the config file to be used!'
	exit()

def FloatCompare(x,y):
	diff = x - y
	if diff < 0:
		return -1
	if diff > 0:
		return 1
	return 0

#This class takes the place of the InputDetection module, it will load up a saved feature list, and pass it to
#the tracker as if it was just calculated, allowing for much faster modifications and adjustments to the tracking without
#having to re-run the background subtraction
class FeatureReader(object):
	def __init__(self,feature_path, start_frame, height, width):
		self.__frame_counter = start_frame
		self.__feature_file = open(feature_path, 'r')
		self.__height = height
		self.__width = width
		#Get header, save to a default input list
		header_line = self.__feature_file.readline()
		header_entries = header_line.rsplit()
		if len(header_entries) == 0 or header_entries[0] == 'Frame':
			print 'Invalid header!'
			print header_entries
			exit()
		self.__template_vector = list()
		for entry in header_entries:
			self.__template_vector.append([entry,''])
		print 'Template Vector:',self.__template_vector#TODO Remove this
		#For the length of the line, take a string and a space, push_back into a list
		current_line = self.__feature_file.readline()
		current_entries = current_line.rsplit()
		#TODO Get next frame number
		if len(current_entries) == 0 or current_entries[0] != 'Frame':
			print 'Invalid File Header'
			print current_entries
			exit()
		#Read "Frame %d", set that number to next frame
		self.__next_frame = int(current_entries[1])
		self.__features_available = True
	def getHeight(self):
		return self.__height
	
	def getWidth(self):
		return self.__width
	
	def getObjects(self):
		if self.__features_available == False:
			print 'Raising no more files exception'
			raise 'No more features available in list!'
		
		if self.__frame_counter > self.__next_frame:
			print "frame_counter > next_frame"
			exit()
		
		print 'Next frame is',self.__next_frame,'current frame is',self.__frame_counter
		#If frame_counter < next_frame, then return an empty list
		if self.__frame_counter < self.__next_frame:
			print 'Skipping frame, next frame is',self.__next_frame,'current frame is',self.__frame_counter
			self.__frame_counter = self.__frame_counter + 1
			return list()
		#If frame_counter == next_frame, then create a list using the following feature vectors until Frame* is encountered, or EOF
		current_entries = ''
		current_feature_vector = list()
		
		#Go Through list of features, matching them with their template dimensions
		while 1:
			current_line = self.__feature_file.readline()
			current_entries = current_line.rsplit()
			if len(current_entries) == 0 or current_entries[0] == 'Frame':
				break
			current_feature = self.__template_vector
			vector_pos = 0
			for entry in current_entries:
				current_feature[vector_pos][1] = entry
				vector_pos = vector_pos + 1
			current_feature_vector.append(current_feature)
		#If the last entry isn't available or isn't what we expect, we set a flag so that the next run will throw an exception
		if len(current_entries) == 0 or current_entries[0] != 'Frame':
			print 'No more features available! (Next call will throw an exception)'
			self.__features_available = False
		self.__next_frame = int(current_entries[1])
		#print 'Current Feature Vector',current_feature_vector
		self.__frame_counter = self.__frame_counter + 1
		return current_feature_vector
	def outputData(self,box_list):
		#TODO Take output list and create output file (possibly boxes)
		return

config_list = InputDetection.ListDim1VecStr()
temp_config_list = ReadConfig(sys.argv[1])
map(PushElement, temp_config_list)

#NOTE Below is an example of the python config reading!
#print 'Returned',find_value(config_list, 'CVMovieFileInput')
#detection = FeatureReader("output/feature.track",1, 213, 320)
detection = InputDetection.InputDetection(config_list)
history_objects = list()
imageHeight = detection.getHeight()
imageWidth = detection.getWidth()
track_log = open('output/track.txt', 'w')
frame_counter = 0
while 1:
	#TODO These constants should be in a config file
	energy_thresh = 100#Since all other weights are set to pass this threshold, it is arbitrarily set to 100
	frame_match_threshold = 25
	box_display_threshold = 1#This should be done in the buffer, so that we don't lose the initial frames gaining confidence
	try:
		current_objects = map(lambda x: map(list,x),list(detection.getObjects()))
	except:
		#TODO Do everything here that needs to be done at the end of execution of the tracking
		print 'No more feature vectors available'
		quit()
		
	current_objects = map(SurveillanceObject, current_objects)
	print 'Frame',frame_counter
	print 'Detection returned',len(current_objects),'candidate objects (may be dropped due to filtering)'
	#-----Do Tracking-------
	#Create a list of objects from the returned list that meets the specifications provided (not too small, not too skinny, etc)
	#current_objects = filter(ViableObject,current_objects)
	print 'Pruning returned',len(current_objects),'objects and we have',len(history_objects),'history objects to match with them'

	#Calculate energy of match between each object in the ascending sorted current_objects list and the history_objects list, store energy and a reference to each object
	match_list = list()

	[CalculateMatchEnergy(x,y) for x in current_objects for y in history_objects]
	match_list.sort(lambda x,y: FloatCompare(x.energy,y.energy))
	print "List Start"

	#Find lowest energy match, remove matches that relate to either of those objects until either our list is empty, or the match energy becomes too high.
	#NOTE This is NOT the best way to perform matching where the goal is to reduce the overall energy;however, those cases are fairly rare and a
	#hungarian matching algorithm can be implemented here when necessary
	for el in xrange(len(match_list)-1):
		element = match_list[el]
		if element.history.matched or element.current.matched or element.energy > energy_thresh:
			continue
		else:
			element.current.matched = True
			element.history.matched = True

		print "ID:",element.history.object_id,"Obj1:(",element.current.valuef("cent_x"),",",element.current.valuef("cent_y"),")",element.current.valuef("area"),"Obj2:(",element.history.valuef("cent_x"),",",element.history.valuef("cent_y"),")",element.history.valuef("area"),"Energy: ",element.energy
		
		#TODO Analyze trends to stereotype behavior
		#TODO When entering, use faster learning
		#These weights represent how fast we include new data into our model
		area_weight = .5
		cent_weight = .8
		mean_gray_weight = .3
		height_weight = .6
		width_weight = .3
		
		#Calculate change in position to aid in position calculation
		element.history.velocity_x = element.current.valuef('cent_x') - element.history.valuef('cent_x')
		element.history.velocity_y = element.current.valuef('cent_y') - element.history.valuef('cent_y')
		
		#Update features
		element.history.value_set('area', (1-area_weight)*element.history.valuef('area') + area_weight*element.current.valuef('area'))
		element.history.value_set('cent_x', (1-cent_weight)*element.history.valuef('cent_x') + cent_weight*element.current.valuef('cent_x'))
		element.history.value_set('cent_y', (1-cent_weight)*element.history.valuef('cent_y') + cent_weight*element.current.valuef('cent_y'))
		element.history.value_set('mean_gray', (1-mean_gray_weight)*element.history.valuef('mean_gray') + mean_gray_weight*element.current.valuef('mean_gray'))
		element.history.value_set('height', (1-height_weight)*element.history.valuef('height') + height_weight*element.current.valuef('height'))
		element.history.value_set('width', (1-width_weight)*element.history.valuef('width') + width_weight*element.current.valuef('width'))
		
		#Update box
		element.history.value_set('tl_x', max([int(element.history.valuef('cent_x') - element.history.valuef('width')/2),0]))
		element.history.value_set('tl_y', max([int(element.history.valuef('cent_y') - element.history.valuef('height')/2),0]))
		element.history.value_set('br_x', min([int(element.history.valuef('cent_x') + element.history.valuef('width')/2),imageWidth - 1]))
		element.history.value_set('br_y', min([int(element.history.valuef('cent_y') + element.history.valuef('height')/2),imageHeight - 1]))
		
		#Update classification
		element.history.value_set('person_probability', element.current.valuef('person_probability'))
		element.history.value_set('car_probability', element.current.valuef('car_probability'))

	
		element.history.frames_since_match = 0
		element.history.frames_matched = element.history.frames_matched + 1
	print "List Stop"
	
	#For all unmatched history objects - Increment unmatched frame counter
	for hist_obj in filter(lambda x: x.matched == False,history_objects):
		hist_obj.frames_since_match = hist_obj.frames_since_match + 1
		if hist_obj.frames_since_match > frame_match_threshold:
			hist_obj.dropped = True

	#TODO For all unmatched current objects
	#for cur_obj in filter(lambda x: x.matched == False,current_objects):
		#TODO Decide if we want to add a stereotype to the object (e.g. if it appears in the middle of the screen, several small objects where one large unmatched one was, etc)
	#DEBUGGING Find all unmatched current objects closest matches
	#for cur_obj in filter(lambda x: x.matched == False):
		
	#Prune history objects that have the dropped set to true
	history_objects = filter(lambda x: x.dropped == False,history_objects)
	
	#Prune current objects that have the dropped set to true
	current_objects = filter(lambda x: x.dropped == False,current_objects)

	#All objects left in current_objects list are added to the history_objects list
	print 'Objects1:',len(history_objects)
	map(lambda x: SurveillanceObject.get_id(x), filter(lambda x: x.matched == False,current_objects))
	history_objects = history_objects + filter(lambda x: x.matched == False,current_objects)
	print 'Objects2:',len(history_objects)
	
	#TODO Heuristics that determine if a box for a given object should be output
	box_list = list()
	for hist_obj in filter(lambda x: x.frames_matched > box_display_threshold,history_objects):
		box_list.append(hist_obj.box_data())
	detection.outputData(box_list)
	#Write boxes to file
	log_list = list()
	for hist_obj in filter(lambda x: x.frames_matched > box_display_threshold,history_objects):
		log_list.append(hist_obj.log_data())


	for box in log_list:
		track_log.write(str(frame_counter)+' '+str(box[0])+' '+str(box[1])+' '+str(box[2])+' '+str(box[3])+' '+str(box[4])+' '+str(box[5])+' '+str(box[6])+'\n')
	frame_counter = frame_counter + 1
	#[x.matched = False for x in history_objects]
	map(lambda x: x.reset_matched(),history_objects)
track_log.close()
