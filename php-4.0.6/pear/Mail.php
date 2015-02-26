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
// | Authors: Chuck Hagenbuch <chuck@horde.org>                           |
// +----------------------------------------------------------------------+

require_once 'PEAR.php';

/**
 * PEAR's Mail:: interface. Defines the interface for implementing
 * mailers under the PEAR hierarchy, and provides supporting functions
 * useful in multiple mailer backends.
 */
class Mail extends PEAR {
    
    /**
     * Provides an interface for generating Mail:: objects of various
     * types
     *
     * @param string The kind of Mail:: object to instantiate.
     * @param array  The parameters to pass to the Mail:: object.
     * @access public
     */
    function factory($driver, $params = array())
    {
	$driver = strtolower($driver);
        if (@include_once 'Mail/' . $driver . '.php') {
            $class = 'Mail_' . $driver;
            return new $class($params);
        } else {
            return new PEAR_Error('Unable to find class for driver ' . $driver);
        }
    }
    
	/**
     * Implements Mail::send() function using php's built-in mail()
     * command.
     * 
     * @param mixed Either a comma-seperated list of recipients
     *              (RFC822 compliant), or an array of recipients,
     *              each RFC822 valid. This may contain recipients not
     *              specified in the headers, for Bcc:, resending
     *              messages, etc.
     *
     * @param array The array of headers to send with the mail, in an
     *              associative array, where the array key is the
     *              header name (ie, 'Subject'), and the array value
     *              is the header value (ie, 'test'). The header
     *              produced from those values would be 'Subject:
     *              test'.
     *
     * @param string The full text of the message body, including any
     *               Mime parts, etc.
     *
     * @return mixed Returns true on success, or a PEAR_Error
     *               containing a descriptive error message on
     *               failure.
     * @access public
     */	
    function send($recipients, $headers, $body)
    {
        // if we're passed an array of recipients, implode it.
        if (is_array($recipients)) {
            $recipients = implode(', ', $recipients);
        }
        
        // get the Subject out of the headers array so that we can
        // pass it as a seperate argument to mail().
        $subject = '';
        if (isset($headers['Subject'])) {
            $subject = $headers['Subject'];
            unset($headers['Subject']);
        }
        
        // flatten the headers out.
        list(,$text_headers) = Mail::prepareHeaders($headers);
        
        return mail($recipients, $subject, $body, $text_headers);
    }
    
    /**
     * Take an array of mail headers and return a string containing
     * text usable in sending a message.
     *
     * @param array The array of headers to prepare, in an associative
     *              array, where the array key is the header name (ie,
     *              'Subject'), and the array value is the header
     *              value (ie, 'test'). The header produced from those
     *              values would be 'Subject: test'.
     *
     * @return mixed Returns false if it encounters a bad address,
     *               otherwise returns an array containing two
     *               elements: Any From: address found in the headers,
     *               and the plain text version of the headers.
     * @access protected
     */
    function prepareHeaders($headers)
    {
        // Look out for the From: value to use along the way.
        $text_headers = '';  // text representation of headers
        $from = null;
        
        foreach ($headers as $key => $val) {
            if ($key == 'From') {
                include_once 'Mail/RFC822.php';
                
                $from_arr = Mail_RFC822::parseAddressList($val, 'localhost', false);
                $from = $from_arr[0]->mailbox . '@' . $from_arr[0]->host;
                if (strstr($from, ' ')) {
                    // Reject outright envelope From addresses with spaces.
                    return false;
                }
                $text_headers .= $key . ': ' . $val . "\n";
            } else if ($key == 'Received') {
                // put Received: headers at the top, since Receieved:
                // after Subject: in the header order is somtimes used
                // as a spam trap.
                $text_headers = $key . ': ' . $val . "\n" . $text_headers;
            } else {
                $text_headers .= $key . ': ' . $val . "\n";
            }
        }
        
        return array($from, $text_headers);
    }
    
    /**
     * Take a set of recipients and parse them, returning an array of
     * bare addresses (forward paths) that can be passed to sendmail
     * or an smtp server with the rcpt to: command.
     *
     * @param mixed Either a comma-seperated list of recipients
     *              (RFC822 compliant), or an array of recipients,
     *              each RFC822 valid.
     *
     * @return array An array of forward paths (bare addresses).
     * @access protected
     */
    function parseRecipients($recipients)
    {
        include_once 'Mail/RFC822.php';
        
        // if we're passed an array, assume addresses are valid and
        // implode them before parsing.
        if (is_array($recipients)) {
            $recipients = implode(', ', $recipients);
        }
        
        // Parse recipients, leaving out all personal info. This is
        // for smtp recipients, etc. All relevant personal information
        // should already be in the headers.
        $addresses = Mail_RFC822::parseAddressList($recipients, 'localhost', false);
        $recipients = array();
        if (is_array($addresses)) {
            foreach ($addresses as $ob) {
                $recipients[] = $ob->mailbox . '@' . $ob->host;
            }
        }
        
        return $recipients;
    }
    
}
?>
