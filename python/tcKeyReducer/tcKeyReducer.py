import os
import re
import sys
import traceback

try:
    from PySide import QtGui, QtCore
    import PySide.QtGui as QtWidgets
    import shiboken
except ImportError:
    from PySide2 import QtGui, QtCore, QtWidgets
    import shiboken2 as shiboken

from maya import cmds
		
GRAPH_EDITOR_NAME = 'graphEditor1GraphEd'
CHANNEL_BOX_NAME = 'mainChannelBox'

def _build_layout(horizontal):
	if (horizontal):
		layout = QtWidgets.QHBoxLayout()
	else:
		layout = QtWidgets.QVBoxLayout()
	layout.setContentsMargins(QtCore.QMargins(2, 2, 2, 2))
	return layout

class LineWidget(QtWidgets.QWidget):
	def __init__(self, label, widget, width=90, parent=None):
		super(LineWidget, self).__init__(parent=parent)

		self.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)

		self._main_layout = _build_layout(True)
		self.setLayout(self._main_layout)

		label = QtWidgets.QLabel(label)
		label.setFixedWidth(width)
		self._main_layout.addWidget(label, 0)
		self._main_layout.addWidget(widget, 0)

def _create_separator(vertical=False):
	separator = QtWidgets.QFrame()
	if vertical:
		separator.setFrameShape(QtWidgets.QFrame.VLine)
	else:
		separator.setFrameShape(QtWidgets.QFrame.HLine)
	separator.setFrameShadow(QtWidgets.QFrame.Sunken)
	return separator		

def _get_icons_path():
	tokens = __file__.split(os.path.sep)
	return os.path.sep.join(tokens[:-1] + ['icons']) + os.path.sep
	
	
class KeyReducerControls(QtWidgets.QWidget):
	def __init__(self,parent=None):
		super(KeyReducerControls, self).__init__(parent)
		self.setWindowTitle('Key Reducer')
		self.setObjectName('Key Reducer')
		
		self._main_layout = _build_layout(False)
		self.setLayout(self._main_layout)
		
		self._script_jobs = []
		
		self._create_widgets()
		self._connect_signals()
		
	def _create_value_layout(self):
		layout = _build_layout(True)
		self._main_layout.addLayout(layout)
		
		v_layout = _build_layout(False)
		self._value_line_edit = QtWidgets.QLineEdit()
		self._value_line_edit.setFixedWidth(80)
		self._value_validator = QtGui.QDoubleValidator()
		self._value_line_edit.setText("0.5")
		self._value_line_edit.setValidator(self._value_validator)
		
		l = LineWidget("Value:", self._value_line_edit, 40)
		l.setFixedWidth(130) 
		v_layout.addWidget(l, 0, QtCore.Qt.AlignTop)
		
		self._pre_bake = QtWidgets.QCheckBox("Pre Bake")
		l = LineWidget("", self._pre_bake, 40)
		v_layout.addWidget(l)
		
		layout.addLayout(v_layout)
		
		v_layout = _build_layout(False)
		layout.addLayout(v_layout)
		self._value_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
		self._value_slider.setMinimum(0.0 * 10000)
		self._value_slider.setMaximum(1.0 * 10000)
		self._value_slider.setValue(0.5 * 10000)
		self._value_slider.setTickPosition(QtWidgets.QSlider.TicksAbove)
		self._value_slider.setTickInterval(1000)
		v_layout.addWidget(self._value_slider)
		
		h_layout = _build_layout(True)
		v_layout.addLayout(h_layout)
		
		self._min_value = QtWidgets.QDoubleSpinBox()
		self._min_value.setValue(0.0)
		self._min_value.setFixedWidth(80)
		self._min_value.setSingleStep(0.1)
		self._min_value.setMaximum(0.4)
		
		l = LineWidget("Min:", self._min_value, 24)
		l.setFixedWidth(115) 
		h_layout.addWidget(l, 0, QtCore.Qt.AlignLeft)
		
		self._max_value = QtWidgets.QDoubleSpinBox()
		self._max_value.setValue(1.0)
		self._max_value.setFixedWidth(80)
		self._max_value.setSingleStep(0.1)
		self._max_value.setMinimum(1.0)
		self._max_value.setMaximum(1000000)
		self._max_value.setMinimum(0.6)
		
		l = LineWidget("Max:", self._max_value, 24)
		l.setFixedWidth(115) 
		h_layout.addWidget(l, 0, QtCore.Qt.AlignRight)
		
	def _create_time_layout(self):
		group = QtWidgets.QGroupBox("Range: ")
		layout = _build_layout(False)
		group.setLayout(layout)
		self._main_layout.addWidget(group)
		
		h_layout = _build_layout(True)
		buttonGroup = QtWidgets.QButtonGroup(self)
		self._entire_button = QtWidgets.QRadioButton("Entire Curve")
		self._entire_button.setChecked(1)
		self._entire_button.setFixedWidth(120)
		self._time_slider_button = QtWidgets.QRadioButton("Time Slider")
		self._time_slider_button.setFixedWidth(120)
		self._custom_range_button = QtWidgets.QRadioButton("Custom")
		h_layout.addWidget(self._entire_button)
		h_layout.addWidget(self._time_slider_button)
		h_layout.addWidget(self._custom_range_button)
		
		layout.addLayout(h_layout)
		
		buttonGroup.addButton(self._entire_button)
		buttonGroup.addButton(self._time_slider_button)
		buttonGroup.addButton(self._custom_range_button)
		
		h_layout = _build_layout(True)
		self._parent_custom_range = QtWidgets.QWidget()
		self._parent_custom_range.setFixedWidth(290)
		self._parent_custom_range.setLayout(h_layout)
		l = QtWidgets.QLabel("Custom: ")
		l.setFixedWidth(60)
		h_layout.addWidget(l)
		self._custom_range_begin = QtWidgets.QSpinBox()
		self._custom_range_begin.setFixedWidth(100)
		min = cmds.playbackOptions(q=True, minTime=True)
		self._custom_range_begin.setValue(min)
		h_layout.addWidget(self._custom_range_begin)
		l = QtWidgets.QLabel("-")
		l.setFixedWidth(10)
		h_layout.addWidget(l)
		self._custom_range_end = QtWidgets.QSpinBox()
		self._custom_range_end.setFixedWidth(100)
		max = cmds.playbackOptions(q=True, maxTime=True)
		self._custom_range_end.setValue(max)
		h_layout.addWidget(self._custom_range_end)
		
		self._parent_custom_range.setEnabled(False)
		
		layout.addWidget(self._parent_custom_range)
		
	def _create_attrs_layout(self):
		group = QtWidgets.QGroupBox("Attributes: ")
		layout = _build_layout(True)
		group.setLayout(layout)
		self._main_layout.addWidget(group)
		
		buttonGroup = QtWidgets.QButtonGroup(self)
		self._all_attrs_button = QtWidgets.QRadioButton("All")
		self._all_attrs_button.setChecked(1)
		self._all_attrs_button.setFixedWidth(120)
		
		self._graph_editor_selection = QtWidgets.QRadioButton("Graph Editor Selection")
		self._graph_editor_selection.setFixedWidth(140)
		
		self._channel_box_selection = QtWidgets.QRadioButton("Channel Box Selection")
		self._channel_box_selection.setFixedWidth(140)
		
		layout.addWidget(self._all_attrs_button)
		layout.addWidget(self._graph_editor_selection)
		layout.addWidget(self._channel_box_selection)
		layout.addWidget(QtWidgets.QWidget(), 1)
		
		buttonGroup.addButton(self._all_attrs_button)
		buttonGroup.addButton(self._graph_editor_selection)
		buttonGroup.addButton(self._channel_box_selection)
	 		
	def _create_widgets(self):		
		self._main_layout.addWidget(_create_separator())
		self._create_value_layout()
		self._main_layout.addWidget(_create_separator())
		self._create_attrs_layout()
		self._main_layout.addWidget(_create_separator())
		self._create_time_layout()
		
		self._main_layout.addWidget(QtWidgets.QWidget(), 1)
	
	def _connect_signals(self):
		self._min_value.valueChanged.connect(self._min_value_changed)
		self._max_value.valueChanged.connect(self._max_value_changed)
		
		self._value_slider.sliderPressed.connect(self._value_slider_pressed)
		self._value_slider.sliderMoved.connect(self._value_slider_moved)
		self._value_slider.sliderReleased.connect(self._value_slider_released)
		
		self._value_line_edit.returnPressed.connect(self._value_line_edit_finished)
		
		self._custom_range_button.toggled.connect(self._custom_range_button_toggled)
				
	def _custom_range_button_toggled(self):
		self._parent_custom_range.setEnabled(self._custom_range_button.isChecked())
				
	def _value_line_edit_finished(self):
		value = float(self._value_line_edit.text())
		if float(value) > self._max_value.value():
			self._value_line_edit.setText(str(self._max_value.value()))
		elif float(value) < self._min_value.value():
			self._value_line_edit.setText(str(self._min_value.value()))
		
		self._value_slider.setValue(float(self._value_line_edit.text()) * 10000)
		
		self._run_command()
	
	def _value_slider_pressed(self):
		cmds.undoInfo(openChunk=True, chunkName='tcKeyReducerDragging')
		try:
			value = float(self._value_slider.value())  / 10000
			self._value_line_edit.setText(str(value))
			
			attrs = self._get_attributes()
			cmds.tcStoreKeys(*attrs)
			self._run_command()
		except:
			traceback.print_exc(file=sys.stdout)
			
	def _value_slider_moved(self):		
		try:
			value = float(self._value_slider.value())  / 10000
			self._value_line_edit.setText(str(value))
			
			cmds.tcRestoreKeys()
			self._run_command()
		except:
			traceback.print_exc(file=sys.stdout)
		
	def _value_slider_released(self):
		try:
			value = float(self._value_slider.value())  / 10000
			self._value_line_edit.setText(str(value))
			
			cmds.tcRestoreKeys()
			self._run_command()
		except:
			traceback.print_exc(file=sys.stdout)
		cmds.undoInfo(closeChunk=True)
			
	def _min_value_changed(self, value):
		self._value_slider.setMinimum(float(value) * 10000)
		self._value_slider.setTickInterval((self._max_value.value() - self._min_value.value()) * 1000)
		
	def _get_attribute_names_from_graph_editor(self):
		animCurves = cmds.animCurveEditor(GRAPH_EDITOR_NAME, q=True, cs=True)
		channels = set()
		for a in animCurves:
			connections = cmds.listConnections("%s.output" % a, s=False, d=True, plugs=True)
			tokens = connections[0].split(".")
			channels.add(str(tokens[len(tokens) - 1]))
		return list(channels)

	def _get_attribute_names_from_channel_box(self):
		channels = set()
		queried_channels =  ([] + 
							(cmds.channelBox(CHANNEL_BOX_NAME, selectedMainAttributes=True, q=True) or []) +  
							(cmds.channelBox(CHANNEL_BOX_NAME, selectedShapeAttributes=True, q=True) or []) +
							(cmds.channelBox(CHANNEL_BOX_NAME, selectedHistoryAttributes=True, q=True) or []) +
							(cmds.channelBox(CHANNEL_BOX_NAME, selectedOutputAttributes=True, q=True) or []))		
		
		for c in queried_channels:
			channels.add(str(c))		
		return list(channels)
	
	def _get_all_attributes(self):
		selection = cmds.ls(sl=True, l=True)
		return [str(c) for c in cmds.listAttr(selection, keyable=True)]

	def _get_attributes(self):
		
		if self._graph_editor_selection.isChecked():
			channels = self._get_attribute_names_from_graph_editor()
		elif self._channel_box_selection.isChecked():
			channels = self._get_attribute_names_from_channel_box()
			if not channels:
				channels = self._get_all_attributes()
		else:
			channels = 	self._get_all_attributes()
			
		ret = []
		for s in cmds.ls(sl=True, l=True):
			for c in channels:
				ret.append(str(s) + "." + c)
		return ret
		
	def _run_command(self):
		attrs = self._get_attributes()
		
		kwargs = {'value': float(self._value_line_edit.text())}
		if (self._pre_bake.isChecked()):
			kwargs['preBake'] = True
		
		if self._time_slider_button.isChecked():
			kwargs['startTime'] = cmds.playbackOptions(q=True, minTime=True)
			kwargs['endTime'] = cmds.playbackOptions(q=True, maxTime=True)
		elif self._custom_range_button.isChecked():
			kwargs['startTime'] = self._custom_range_begin.value()
			kwargs['endTime'] = self._custom_range_end.value()
		
		cmds.tcKeyReducer(*attrs, **kwargs)
		
	def _max_value_changed(self, value):
		self._value_slider.setMaximum(float(value) * 10000)
		self._value_slider.setTickInterval((self._max_value.value() - self._min_value.value()) * 1000)
	
	
class KeyReducerWidget(QtWidgets.QDialog):
	def __init__(self,parent=None):
		super(KeyReducerWidget, self).__init__(parent)
		self.setWindowTitle('Key Reducer')
		self.setObjectName('Key Reducer')
		
		self._main_layout = _build_layout(False)
		self.setLayout(self._main_layout)
		
		about = QtWidgets.QLabel()
		about.setPixmap(QtGui.QPixmap(_get_icons_path()+'abouts.png'))
		self._main_layout.addWidget(about)
		
		self._controls = KeyReducerControls()
		self._main_layout.addWidget(self._controls)
		
		self._script_jobs = []
		
	def _selection_changed(self):
		selection = cmds.ls(sl=True, l=True)
		self._controls.setEnabled(bool(selection))
		
	def _start_script_jobs(self):
		self._script_jobs = []
		id = cmds.scriptJob(event=["SelectionChanged", self._selection_changed])
		self._script_jobs.append(id)
	
	def closeEvent(self, event):
		for id in self._script_jobs:
			if cmds.scriptJob(exists=id):
				cmds.scriptJob(kill=id)	
		
		self._script_jobs = []
		super(KeyReducerWidget, self).closeEvent(event)
	
key_reducer_widget = None
def get_key_reducer_widget():
	global key_reducer_widget
	if key_reducer_widget is None:
		key_reducer_widget = KeyReducerWidget()
	return key_reducer_widget


def run():
	if not cmds.pluginInfo('tcKeyReducer', q=1, l=1):
		cmds.loadPlugin('tcKeyReducer', quiet=True)
	
	w = get_key_reducer_widget()
	w._start_script_jobs()
	w._selection_changed()
	w.resize(500, 200)
	w.show()
	w.raise_()



