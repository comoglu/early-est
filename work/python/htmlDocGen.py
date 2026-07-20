# htmlDoc.py - Generate HTML documents

# Version 1.0 - November 30, 1995

# Written by:
#	 Joel Shprentz
#	 BDM Federal, Inc.
#	 1501 BDM Way
#	 McLean, VA 22102
#	 shprentz@bdm.com

# 20110616 AJL - downloaded from http://www.python.org/workshops/1995-12/papers/shprentz.html
# 20110617 AJL - added attributes to Tag.IMG
# 20210916 AJL - converted to Python 3(3.8)


from string import *
from sys import stdout
import types

# Augmented output file for rendering HTML
class HtmlFile:

	maxLine = 70

	def __init__(self, aFile):
		self.outfile = aFile
		self.preformatted = 0
		self.charCount = 0
		self.lastWhite = 1
		self.newline = 1

	def startPreformatted(self):
		self.preformatted = self.preformatted + 1

	def endPreformatted(self):
		if self.preformatted > 0:
			self.preformatted = self.preformatted - 1

	def writePreformatted(self, text):
		self.outfile.write(text)
		self.charCount = HtmlFile.maxLine
		self.lastWhite = 0
		self.newline = 0

	def writeNewline(self):
		if self.newline == 0:
			self.outfile.write("\n")
			self.charCount = 0
			self.lastWhite = 1
			self.newline = 1

	def writeWhiteSpace(self):
		if self.lastWhite == 0:
			if self.charCount < HtmlFile.maxLine:
				self.outfile.write(" ")
				self.charCount = self.charCount + 1
				self.lastWhite = 1
			else:
				self.writeNewline()

	def writeUnsplittable(self, text):
		size = len(text)
		if size > 0:
			#if self.lastWhite and self.charCount + size > HtmlFile.maxLine:
				#self.writeNewline()
			self.outfile.write(text)
			self.charCount = self.charCount + size
			self.lastWhite = 0
			self.newline = 0

	def writeSplitText(self, text):
		if len(text) > 0:
			if len(text[0].strip()) == 0:
				self.writeWhiteSpace()
			words=text.split()
			if len(words) > 0:
				self.writeUnsplittable(words[0])
				if len(words) > 1:
					for aWord in words [1:]:
						self.writeWhiteSpace()
						self.writeUnsplittable(aWord)
				if len(text[len(text) - 1].strip()) == 0:
					self.writeWhiteSpace()

	def write(self, text):
		if self.preformatted:
			self.writePreformatted(text)
		else:
			self.writeSplitText(text)

	def writeSpace(self):
		if self.preformatted:
			self.writePreformatted(" ")
		else:
			self.writeWhiteSpace()

	def writeAsIs(self, text):
		if self.preformatted:
			self.writePreformatted(" ")
		else:
			self.writeUnsplittable(text)

# Attributes of tags

class HtmlAttribute:		

	def __init__(self, aName, aNeedQuotes = 1):
		self.name = aName
		self.needQuotes = aNeedQuotes

	def writeToHtml(self, outfile, values):
		if self.name in values:
			value = values [self.name]
			outfile.writeSpace()
			outfile.writeAsIs(self.name)
			if value != None:
				if self.needQuotes:
					outfile.writeAsIs('="%s"' % value)
				else:
					outfile.writeAsIs('=%s' % value)


# Parent class for all HTML contents(abstract class)
class HtmlContents:

	def writeHtml(self, outfile):
		self.writeToHtml(HtmlFile(outfile))

	def writeToHtml(self, Htmlout):
		raise "HTMLException Implemented by subclass"

	def setFormValues(self, valueDict):
		pass


# Whitespace contents - for efficiency, use theSpace
class HtmlSpace(HtmlContents):

	def writeToHtml(self, outfile):
		outfile.writeSpace()

theSpace = HtmlSpace()



# Text contents - converts to valid HTML on output
class HtmlText(HtmlContents):

	def __init__(self, someText):
		self.text = someText

	def writeToHtml(self, outfile):
		pieces = self.text.split("&")
		outText =("&amp;").join(pieces)
		pieces = outText.split("<")
		outText =("&lt;").join(pieces)
		pieces = outText.split(">")
		outText =("&gt;").join(pieces)
		# fix places where there should be a &
		pieces = outText.split("&amp;nbsp;")
		outText =("&nbsp;").join(pieces)
		pieces = outText.split("&amp;deg;")
		outText =("&deg;").join(pieces)
		pieces = outText.split("&amp;&amp;")
		outText =("&").join(pieces)
		pieces = outText.split("&gt;&gt;")
		outText =(">").join(pieces)
		pieces = outText.split("&lt;&lt;")
		outText =("<").join(pieces)
		outfile.write(outText)



# Tagged elements - contains start tag, attributes, contents, and end tag
class HtmlElement(HtmlContents):

	def __init__(self, aTag, someContents = None, needEnd = 1, endLine = 0,
					 someDefaultAttributes = [], attributeDict = {}):
		self.tag = aTag
		self.needEndTag = needEnd
		self.tagEndsLine = endLine
		self.setContents(someContents)
		self.defaultAttributes = someDefaultAttributes
		self.attributes = attributeDict


	def append(self, anItem):
		#print("DEBUG: type(anItem):", type(anItem))
		if type(anItem) == bytes:
			self.contents.append(HtmlText(anItem))
		elif type(anItem) == str:
			self.contents.append(HtmlText(anItem))
		elif type(anItem) == list:
			self.appendListContents(anItem)
		elif anItem:
			self.contents.append(anItem)

	def appendListContents(self, aList):
		for anItem in aList:
			self.append(anItem)

	def setContents(self, anItem):
		self.contents = []
		self.append(anItem)

	def prepend(self, anItem):
		if type(anItem) == bytes:
			self.contents.insert(0, HtmlText(anItem))
		elif type(anItem) == list:
			self.prependListContents(anItem)
		elif anItem:
			self.contents.insert(0, anItem)

	def prependListContents(self, aList):
		for anItem in aList.reverse():
			self.prepend(anItem)

	def getAttribute(self, attributeName):
		return self.attributes [attributeName]

	def setAttribute(self, attributeName, attributeValue):
		self.attributes [attributeName] = attributeValue

	def writeStartTag(self, outfile):
		outfile.write("<%s" % self.tag)
		for anAttribute in self.defaultAttributes:
			anAttribute.writeToHtml(outfile, self.attributes)
		outfile.write(">")

	def writeContents(self, outfile):
		for element in self.contents:
			#print("DEBUG:", element) # DEBUG
			element.writeToHtml(outfile)

	def writeToHtml(self, outfile):
		self.writeStartTag(outfile)
		self.writeContents(outfile)
		if self.needEndTag:
			outfile.write("</%s>" % self.tag)
		if self.tagEndsLine:
			outfile.writeNewline()

	def setFormValues(self, valueDict):
		for element in self.contents:
			element.setFormValues(valueDict)


# Class of comment HTML elements
class HtmlCommentElement(HtmlElement):

	def writeContents(self, outfile):
		outfile.startPreformatted()
		HtmlElement.writeContents(self, outfile)
		outfile.endPreformatted()

	def writeToHtml(self, outfile):
		outfile.writeNewline()
		outfile.write("<!--")
		self.writeContents(outfile)
		outfile.writeNewline()
		outfile.write("-->")
		outfile.writeNewline()


# Class of preformatted HTML elements
class HtmlPreformattedElement(HtmlElement):

	def writeContents(self, outfile):
		outfile.startPreformatted()
		HtmlElement.writeContents(self, outfile)
		outfile.endPreformatted()


# Class of text-like form input fields
class HtmlFormInputText(HtmlElement):

	def __init__(self, type, name, value = None, size = None, maxlength = None):
		attributes = {"TYPE": type, "NAME": name}
		if value:
			attributes ["VALUE"] = value
		if size:
			attributes ["SIZE"] = size
		if maxlength:
			attributes ["MAXLENGTH"] = maxlength
		HtmlElement.__init__(self, "INPUT", None, 0, 0,
								  HtmlFormInputText.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("TYPE", 0),
		HtmlAttribute("NAME", 0),
		HtmlAttribute("SIZE", 0),
		HtmlAttribute("MAXLENGTH", 0),
		HtmlAttribute("VALUE")
		]

	def setFormValues(self, valueDict):
		try:
			values = valueDict [self.getAttribute("NAME")]
			self.setAttribute("VALUE", values [0])
		except:
			pass


# Class text area fields
class HtmlFormTextArea(HtmlPreformattedElement):

	def __init__(self, name, cols, rows, contents = None):
		attributes = {"NAME": name, "COLS": cols, "ROWS": rows}
		HtmlPreformattedElement.__init__(self, "TEXTAREA", contents, 1, 0,
								  HtmlFormTextArea.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("NAME", 0),
		HtmlAttribute("COLS", 0),
		HtmlAttribute("ROWS", 0)
		]
	
	def setFormValues(self, valueDict):
		try:
			values = valueDict [self.getAttribute("NAME")]
			self.setContents(values [0])
		except:
			pass


# Class of checkbox and radio button form input fields
class HtmlFormInputCheckBox(HtmlElement):

	def __init__(self, type, name, value, contents = None, checked = 0):
		attributes = {"TYPE": type, "NAME": name, "VALUE": value}
		if checked:
			attributes ["CHECKED"] = None
		HtmlElement.__init__(self, "INPUT", contents, 0, 0,
								  HtmlFormInputCheckBox.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("TYPE", 0),
		HtmlAttribute("NAME", 0),
		HtmlAttribute("CHECKED"),
		HtmlAttribute("VALUE")
		]

	def setFormValues(self, valueDict):
		try:
			values = valueDict [self.getAttribute("NAME")]
			if values.count(self.getAttribute("VALUE")) > 0:
				self.setAttribute("CHECKED", None)
		except:
			pass


# Class of select lists
class HtmlFormSelect(HtmlElement):

	def __init__(self, name, contents = None, multiple = 0, size = None):
		attributes = {"NAME": name}
		if multiple:
			attributes ["MULTIPLE"] = None
		if size:
			attributes ["SIZE"] = size
		HtmlElement.__init__(self, "SELECT", contents, 1, 0,
								  HtmlFormSelect.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("TYPE", 0),
		HtmlAttribute("NAME", 0),
		HtmlAttribute("MULTIPLE"),
		HtmlAttribute("SIZE", 0)
		]


	def setFormValues(self, valueDict):
		try:
			values = valueDict [self.getAttribute("NAME")]
			for element in self.contents:
				element.setOptionValues(values)
		except:
			pass

# Class of select list options
class HtmlFormOption(HtmlElement):

	def __init__(self, contents = None, value = None, selected = 0):
		attributes = {}
		if value:
			attributes ["VALUE"] = value
		if selected:
			attributes ["SELECTED"] = None
		HtmlElement.__init__(self, "OPTION", contents, 0, 1,
								  HtmlFormOption.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("VALUE"),
		HtmlAttribute("SELECTED")
		]


	def setOptionValues(self, values):
		try:
			if values.count(self.getAttribute("VALUE")) > 0:
				self.setAttribute("SELECTED", None)
		except:
			pass

# Class of image imput fields
class HtmlFormInputImage(HtmlElement):

	def __init__(self, name, source, align = "top"):
		attributes = {"TYPE": "IMAGE", "NAME": name, "SRC": source, "ALIGN": align}
		HtmlElement.__init__(self, "INPUT", None, 0, 0,
								  HtmlFormInputImage.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("TYPE", 0),
		HtmlAttribute("NAME", 0),
		HtmlAttribute("SRC"),
		HtmlAttribute("ALIGN")
		]


# Class for submit and reset buttons
class HtmlFormInputButton(HtmlElement):

	def __init__(self, type, name = None, value = None):
		attributes = {"TYPE": type}
		if name:
			attributes ["NAME"] = name
		if value:
			attributes ["VALUE"] = value
		HtmlElement.__init__(self, "INPUT", None, 0, 0,
								  HtmlFormInputButton.fieldAttributes, attributes)

	fieldAttributes = [
		HtmlAttribute("TYPE", 0),
		HtmlAttribute("NAME", 0),
		HtmlAttribute("VALUE")
		]


# HTML Tag Factory Class
class HtmlTagFactory:


	styleAttributes = [
		HtmlAttribute("TYPE"),
		]
	def STYLE(self, type = None):
		attributes = { "TYPE": type }
		return HtmlElement("STYLE", None, 1, 1, HtmlTagFactory.styleAttributes, attributes)

	def COMMENT(self, contents = None):
		return HtmlCommentElement("COMMENT", contents)


	# These tags end with a newline
	sectionAttributes = [
		HtmlAttribute("CLASS"),
		]
	def section(self, tag, contents = None, tag_class = None):
		attributes = {}
		if tag_class != None:
			attributes ["CLASS"] = tag_class
		return HtmlElement(tag, contents, 1, 1, HtmlTagFactory.sectionAttributes, attributes)

	def HTML(self, contents = None):
		return self.section("HTML", contents)

	def HEAD(self, contents = None):
		return self.section("HEAD", contents)

	def TITLE(self, contents = None):
		return self.section("TITLE", contents)

	def BODY(self, contents = None, tag_class = None):
		return self.section("BODY", contents, tag_class)

	def FONT(self, contents = None, tag_class = None):
		return self.section("FONT", contents, tag_class)

	def P(self, contents = None, tag_class = None):
		return self.section("P", contents, tag_class)

	def ADDR(self, contents = None):
		return self.section("ADDR", contents)

	def BLOCKQUOTE(self, contents = None):
		return self.section("BLOCKQUOTE", contents)

	def UL(self, contents = None, tag_class = None):
		return self.section("UL", contents, tag_class)

	def OL(self, contents = None, tag_class = None):
		return self.section("OL", contents, tag_class)

	def LI(self, contents = None, tag_class = None):
		return self.section("LI", contents, tag_class)

	def DL(self, contents = None, tag_class = None):
		return self.section("DL", contents, tag_class)

	# Definition list terms and definitions
	def DT(self, contents = None, tag_class = None):
		return self.section("DT", contents, tag_class)

	def DD(self, contents = None, tag_class = None):
		return self.section("DD", contents, tag_class)


	# table
	def TABLE(self, contents = None, tag_class = None):
		return self.section("TABLE", contents, tag_class)

	def CAPTION(self, contents = None, tag_class = None):
		return self.section("CAPTION", contents, tag_class)

	def TH(self, contents = None, tag_class = None):
		return self.section("TH", contents, tag_class)

	def TR(self, contents = None, tag_class = None):
		return self.section("TR", contents, tag_class)

	def TD(self, contents = None, tag_class = None):
		return self.section("TD", contents, tag_class)


	# Preformatted sections
	def preformatted(self, tag, contents = None):
		return HtmlPreformattedElement(tag, contents, 1, 1)

	def PRE(self, contents = None):
		return self.preformatted("PRE", contents)


	# The heading tags
	sectionAttributes = [
		HtmlAttribute("CLASS"),
		]
	def heading(self, tag, contents = None, tag_class = None):
		attributes = {}
		if tag_class != None:
			attributes ["CLASS"] = tag_class
		return HtmlElement(tag, contents, 1, 1, HtmlTagFactory.sectionAttributes, attributes)

	def H1(self, contents = None, tag_class = None):
		return self.heading("H1", contents, tag_class)

	def H2(self, contents = None, tag_class = None):
		return self.heading("H2", contents, tag_class)

	def H3(self, contents = None, tag_class = None):
		return self.heading("H3", contents, tag_class)

	def H4(self, contents = None, tag_class = None):
		return self.heading("H4", contents, tag_class)

	def H5(self, contents = None, tag_class = None):
		return self.heading("H5", contents, tag_class)

	def H6(self, contents = None, tag_class = None):
		return self.heading("H6", contents, tag_class)

	# These tags mark phrases. They do not end the line.
	def phrase(self, tag, contents = None):
		return HtmlElement(tag, contents, 1, 0)

	def B(self, contents = None):
		return self.phrase("B", contents)

	def I(self, contents = None):
		return self.phrase("I", contents)

	def TT(self, contents = None):
		return self.phrase("TT", contents)

	def CITE(self, contents = None):
		return self.phrase("CITE", contents)

	def CODE(self, contents = None):
		return self.phrase("CODE", contents)

	def EM(self, contents = None):
		return self.phrase("EM", contents)

	def KBD(self, contents = None):
		return self.phrase("KBD", contents)

	def SAMP(self, contents = None):
		return self.phrase("SAMP", contents)

	def STRONG(self, contents = None):
		return self.phrase("STRONG", contents)

	def VAR(self, contents = None):
		return self.phrase("VAR", contents)

	# Miscellaneous tags
	def BR(self):
		return HtmlElement("BR", None, 0, 1)

	def HR(self):
		return HtmlElement("HR", None, 0, 1)

	# Anchor may have HREF and NAME attributes
	anchorAttributes = [
		HtmlAttribute("HREF"),
		HtmlAttribute("NAME"),
		HtmlAttribute("TARGET"),
		HtmlAttribute("TITLE")
		]

	def A(self, contents = None, href = None, name = None, title = None, target = None):
		attributes = {}
		if href != None:
			attributes ["HREF"] = href
		if name != None:
			attributes ["NAME"] = name
		if target != None:
			attributes ["TARGET"] = target
		if title != None:
			attributes ["TITLE"] = title
		return HtmlElement("A", contents, 1, 0, HtmlTagFactory.anchorAttributes, attributes)

	 # Image may have source and ISMAP attributes
	imageAttributes = [
		HtmlAttribute("SRC"),
		HtmlAttribute("ALIGN"),
		HtmlAttribute("HEIGHT"),
		HtmlAttribute("WIDTH"),
		HtmlAttribute("ALT"),
		HtmlAttribute("ISMAP")
		]

	def IMG(self, source, alternate, align = None, ismap = 0, height = None, width = None):
		attributes = { "SRC": source, "ALT": alternate}
		if align != None:
			attributes ["ALIGN"] = align
		if ismap:
			attributes ["ISMAP"] = None
		if height != None:
			attributes ["HEIGHT"] = height
		if width != None:
			attributes ["WIDTH"] = width
		return HtmlElement("IMG", None, 0, 0, HtmlTagFactory.imageAttributes, attributes)

	# Form has action and method attributes
	formAttributes = [
		HtmlAttribute("ACTION"),
		HtmlAttribute("METHOD", 0),
		]

	def FORM(self, contents = None, action = None, method = "GET"):
		attributes = { "METHOD": method}
		if action:
			attributes ["ACTION"] = action
		return HtmlElement("FORM", contents, 1, 0, HtmlTagFactory.formAttributes, attributes)

	# Form fields have special classes
	def inputText(self, name, value = None, size = None, maxlength = None):
			return HtmlFormInputText("TEXT", name, value, size, maxlength)

	def inputPassword(self, name, value = None, size = None, maxlength = None):
			return HtmlFormInputText("PASSWORD", name, value, size, maxlength)

	def inputHidden(self, name, value = ""):
			return HtmlFormInputText("HIDDEN", name, value)

	def inputCheckBox(self, name, value, contents = None, checked = 0):
			return HtmlFormInputCheckBox("CHECKBOX", name, value, contents, checked)

	def inputRadioButton(self, name, value, contents = None, checked = 0):
			return HtmlFormInputCheckBox("RADIO", name, value, contents, checked)

	def inputImage(self, name, source, align = "top"):
			return HtmlFormInputImage(name, source, align)

	def inputSubmit(self, name = None, value = "Submit"):
			return HtmlFormInputButton("SUBMIT", name, value)

	def inputReset(self, value = "Reset"):
			return HtmlFormInputButton("RESET", None, value)

	def SELECT(self, name, contents = None, multiple = 0, size = None):
			return HtmlFormSelect(name, contents, multiple, size)

	def OPTION(self, contents = None, value = None, selected = 0):
			return HtmlFormOption(contents, value, selected)

	def TEXTAREA(self, name, cols, rows, contents = None):
			return HtmlFormTextArea(name, cols, rows, contents)

Tag = HtmlTagFactory()

# HTML Document
class HtmlDocument:

	def __init__(self):
		self.htmlTitle = None
		self.contents = []

	def setHtmlTitle(self, text):
		self.htmlTitle = text

	def prepend(self, moreContents):
		self.contents.insert(0, moreContents)

	def append(self, moreContents):
		self.contents.append(moreContents)

	# These methods construct various parts of the document
	def headTitle(self):
		if self.htmlTitle: 
			return Tag.TITLE(self.htmlTitle)
		else:
			return Tag.TITLE("Untitled document")

	def head(self):
		return Tag.HEAD(self.headTitle())

	def startBody(self):
		return None

	def endBody(self):
		return None

	def setFormValues(self, valueDict):
		for element in self.contents:
			element.setFormValues(valueDict)

	def writeHtml(self, outfile):
		body = Tag.BODY([self.startBody(), self.contents, self.endBody()])
		doc = Tag.HTML([self.head(), body])
		doc.writeHtml(outfile)

	def printHtml(self):
		self.writeHtml(stdout)
