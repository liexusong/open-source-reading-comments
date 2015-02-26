<?php
//
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2001 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.02 of the PHP license,      |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Stig Bakken <ssb@fast.no>                                   |
// |                                                                      |
// +----------------------------------------------------------------------+
//

require_once "PEAR.php";

/**
 * Administration class used to make a PEAR release tarball.
 *
 * @since PHP 4.0.2
 * @author Stig Bakken <ssb@fast.no>
 */
class PEAR_Packager extends PEAR
{
    // {{{ properties

    /** XML_Parser object */
    var $parser;

    /** stack of elements, gives some sort of XML context */
    var $element_stack;

    /** name of currently parsed XML element */
    var $current_element;

    /** array of attributes of the currently parsed XML element */
    var $current_attributes = array();

    /** assoc with information about the package */
    var $pkginfo = array();

    /** name of the package directory, for example Foo-1.0 */
    var $pkgdir;

    /** directory where PHP code files go */
    var $phpdir;

    /** directory where PHP extension files go */
    var $extdir;

    /** directory where documentation goes */
    var $docdir;

    /** directory where system state information goes */
    var $statedir;

    /** debug mode (integer) */
    var $debug = 0;

    /** temporary directory */
    var $tmpdir;

    /** whether file list is currently being copied */
    var $recordfilelist;

    /** temporary space for copying file list */
    var $filelist;

    /** package name and version, for example "HTTP-1.0" */
    var $pkgver;

    // }}}

    // {{{ constructor

    function PEAR_Packager($phpdir = PEAR_INSTALL_DIR,
                           $extdir = PEAR_EXTENSION_DIR,
                           $docdir = '')
    {
        $this->PEAR();
        $this->phpdir = $phpdir;
        $this->extdir = $extdir;
        $this->docdir = $docdir;
    }

    // }}}
    // {{{ destructor

    function _PEAR_Packager() {
	$this->_PEAR();
        while (is_array($this->_tempfiles) &&
               $file = array_shift($this->_tempfiles))
        {
            if (is_dir($file)) {
                system("rm -rf $file"); // XXX FIXME Windows
            } else {
                unlink($file);
            }
        }
    }

    // }}}

    function Package($pkgfile = "package.xml")
    {
        $pwd = getcwd();
        $fp = @fopen($pkgfile, "r");
        if (!is_resource($fp)) {
            return $this->raiseError($php_errormsg);
        }
        
	$xp = xml_parser_create();
	if (!$xp) {
	    return $this->raiseError("Unable to create XML parser.");
	}
	xml_set_object($xp, $this);
	xml_set_element_handler($xp, "startHandler", "endHandler");
	xml_set_character_data_handler($xp, "charHandler");
	xml_parser_set_option($xp, XML_OPTION_CASE_FOLDING, false);
	xml_parser_set_option($xp, XML_OPTION_TARGET_ENCODING, "UTF-8");

	$this->element_stack = array();
	$this->pkginfo = array();
	$this->current_element = false;
        
        $data = fread($fp, filesize($pkgfile));
        fclose($fp);
        if (!xml_parse($xp, $data, true)) {
            $msg = sprintf("XML error: %s at line %d",
                           xml_error_string(xml_get_error_code($xp)),
                           xml_get_current_line_number($xp));
            xml_parser_free($xp);
            return $this->raiseError($msg);
	}
	xml_parser_free($xp);

        $pkginfofile = $this->tmpdir . DIRECTORY_SEPARATOR . "package.xml";
        $fp = fopen($pkginfofile, "w");
        if (!is_resource($fp)) {
            return $this->raiseError("Could not create $pkginfofile: $php_errormsg");
        }

        $this->filelist = preg_replace('/^[\r\n]+\s+/', '    ', $this->filelist);
        $this->filelist = preg_replace('/\n\s+/', "\n    ", $this->filelist);
        $this->filelist = preg_replace('/\n\s+$/', "", $this->filelist);

        fputs($fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n".
              "<!DOCTYPE Package PUBLIC \"-//PHP Group//DTD PEAR Package 1.0//EN//XML\" \"http://php.net/pear/package.dtd\">\n".
              "<Package Type=\"Source\">\n".
              "  <Name>".$this->pkginfo["Package,Name"]."</Name>\n".
              "  <Summary>".$this->pkginfo["Package,Summary"]."</Summary>\n".
              "  <Maintainer>\n".
              "    <Initials>".$this->pkginfo["Maintainer,Initials"]."</Initials>\n".
              "    <Name>".$this->pkginfo["Maintainer,Name"]."</Name>\n".
              "    <Email>".$this->pkginfo["Maintainer,Email"]."</Email>\n".
              "  </Maintainer>\n".
              "  <Release>\n".
              "    <Version>".$this->pkginfo["Release,Version"]."</Version>\n".
              "    <Date>".$this->pkginfo["Release,Date"]."</Date>\n".
              "    <Notes>".$this->pkginfo["Release,Notes"]."</Notes>\n".
              "  </Release>\n".
              "  <FileList>\n".
              "$this->filelist\n".
              "  </FileList>\n".
              "</Package>\n");
        fclose($fp);
        chdir(dirname($this->tmpdir));
        // XXX FIXME Windows and non-GNU tar
        $pkgver = quotemeta($this->pkgver);
        system("tar -cvzf $pwd/${pkgver}.tgz $pkgver");
    }

    // {{{ mkDirHier()

    function mkDirHier($dir)
    {
        $dirstack = array();
        while (!is_dir($dir) && $dir != DIRECTORY_SEPARATOR) {
            array_unshift($dirstack, $dir);
            $dir = dirname($dir);
        }
        while ($newdir = array_shift($dirstack)) {
            if (mkdir($newdir, 0777)) {
                $this->log(1, "created dir $newdir");
            } else {
                return $this->raiseError("mkdir($newdir) failed");
            }
        }
    }

    // }}}
    // {{{ log()

    function log($level, $msg)
    {
        if ($this->debug >= $level) {
            print "$msg\n";
        }
    }

    // }}}

    // {{{ startHandler()

    function startHandler($xp, $name, $attribs)
    {
	array_push($this->element_stack, $name);
	$this->current_element = $name;
	$this->current_attributes = $attribs;
        $this->tmpdata = '';
        if ($this->recordfilelist) {
            $this->filelist .= "<$name";
            foreach ($attribs as $k => $v) {
                $this->filelist .= " $k=\"$v\"";
            }
            $this->filelist .= ">";
        }
	switch ($name) {
	    case "Package":
                if ($attribs["Type"]) {
                    // warning
                }
		break;
            case "FileList":
                // XXX FIXME Windows
                $this->recordfilelist = true;
                $pwd = getcwd();
                $this->pkgver = $this->pkginfo["Package,Name"] . "-" .
                    $this->pkginfo["Release,Version"];
                $this->tmpdir = $pwd . DIRECTORY_SEPARATOR . $this->pkgver;
                if (file_exists($this->tmpdir)) {
                    xml_parser_free($xp);
                    $this->raiseError("$this->tmpdir already exists",
                                      null, PEAR_ERROR_TRIGGER,
                                      E_USER_ERROR);
                }
                if (!mkdir($this->tmpdir, 0755)) {
                    xml_parser_free($xp);
                    $this->raiseError("Unable to create temporary directory $this->tmpdir.",
                                      null, PEAR_ERROR_TRIGGER,
                                      E_USER_ERROR);
                }
                $this->_tempfiles[] = $this->tmpdir;
                break;
	}
    }

    // }}}
    // {{{ endHandler()

    function endHandler($xp, $name)
    {
	array_pop($this->element_stack);
	$this->current_element = $this->element_stack[sizeof($this->element_stack)-1];
        switch ($name) {
            case "FileList":
                $this->recordfilelist = false;
                break;
        }
        if ($this->recordfilelist) {
            $this->filelist .= "</$name>";
        }
    }

    // }}}
    // {{{ charHandler()

    function charHandler($xp, $data)
    {
        if ($this->recordfilelist) {
            $this->filelist .= $data;
        }
	switch ($this->current_element) {
	    case "Dir":
		break;
	    case "File":
                $file = "$this->tmpdir/$data";
                $dir = dirname($file);
                if (!is_dir($dir)) {
                    if (!$this->mkDirHier($dir)) {
                        $this->log(0, "could not mkdir $dir");
                        break;
                    }
                }
                if (!@copy($data, $file)) {
                    $this->log(0, "could not copy $data to $file");
                }
                // fall through
            default:
                $data = trim($data);
                if ($data) {
                    $id = implode(",", array_slice($this->element_stack, -2));
                    $this->pkginfo[$id] = $data;
                }
                break;
	}
    }

    // }}}
}

?>
