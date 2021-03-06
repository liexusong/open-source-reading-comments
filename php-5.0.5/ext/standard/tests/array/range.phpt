--TEST--
range()
--FILE--
<?php
	var_dump(range(1, 100));
	var_dump(range(100, 1));
	
	var_dump(range("1", "100"));
	var_dump(range("100", "1"));
	
	var_dump(range("a", "z"));
	var_dump(range("z", "a"));
	var_dump(range("q", "q"));
	
	var_dump(range(5, 5));
	
	var_dump(range(5.1, 10.1));
	var_dump(range(10.1, 5.1));
	
	var_dump(range("5.1", "10.1"));
	var_dump(range("10.1", "5.1"));
	
	var_dump(range(1, 5, 0.1));
	var_dump(range(5, 1, 0.1));
	
	var_dump(range(1, 5, "0.1"));
	var_dump(range("1", "5", 0.1));
?>
--EXPECT--
array(100) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
  [5]=>
  int(6)
  [6]=>
  int(7)
  [7]=>
  int(8)
  [8]=>
  int(9)
  [9]=>
  int(10)
  [10]=>
  int(11)
  [11]=>
  int(12)
  [12]=>
  int(13)
  [13]=>
  int(14)
  [14]=>
  int(15)
  [15]=>
  int(16)
  [16]=>
  int(17)
  [17]=>
  int(18)
  [18]=>
  int(19)
  [19]=>
  int(20)
  [20]=>
  int(21)
  [21]=>
  int(22)
  [22]=>
  int(23)
  [23]=>
  int(24)
  [24]=>
  int(25)
  [25]=>
  int(26)
  [26]=>
  int(27)
  [27]=>
  int(28)
  [28]=>
  int(29)
  [29]=>
  int(30)
  [30]=>
  int(31)
  [31]=>
  int(32)
  [32]=>
  int(33)
  [33]=>
  int(34)
  [34]=>
  int(35)
  [35]=>
  int(36)
  [36]=>
  int(37)
  [37]=>
  int(38)
  [38]=>
  int(39)
  [39]=>
  int(40)
  [40]=>
  int(41)
  [41]=>
  int(42)
  [42]=>
  int(43)
  [43]=>
  int(44)
  [44]=>
  int(45)
  [45]=>
  int(46)
  [46]=>
  int(47)
  [47]=>
  int(48)
  [48]=>
  int(49)
  [49]=>
  int(50)
  [50]=>
  int(51)
  [51]=>
  int(52)
  [52]=>
  int(53)
  [53]=>
  int(54)
  [54]=>
  int(55)
  [55]=>
  int(56)
  [56]=>
  int(57)
  [57]=>
  int(58)
  [58]=>
  int(59)
  [59]=>
  int(60)
  [60]=>
  int(61)
  [61]=>
  int(62)
  [62]=>
  int(63)
  [63]=>
  int(64)
  [64]=>
  int(65)
  [65]=>
  int(66)
  [66]=>
  int(67)
  [67]=>
  int(68)
  [68]=>
  int(69)
  [69]=>
  int(70)
  [70]=>
  int(71)
  [71]=>
  int(72)
  [72]=>
  int(73)
  [73]=>
  int(74)
  [74]=>
  int(75)
  [75]=>
  int(76)
  [76]=>
  int(77)
  [77]=>
  int(78)
  [78]=>
  int(79)
  [79]=>
  int(80)
  [80]=>
  int(81)
  [81]=>
  int(82)
  [82]=>
  int(83)
  [83]=>
  int(84)
  [84]=>
  int(85)
  [85]=>
  int(86)
  [86]=>
  int(87)
  [87]=>
  int(88)
  [88]=>
  int(89)
  [89]=>
  int(90)
  [90]=>
  int(91)
  [91]=>
  int(92)
  [92]=>
  int(93)
  [93]=>
  int(94)
  [94]=>
  int(95)
  [95]=>
  int(96)
  [96]=>
  int(97)
  [97]=>
  int(98)
  [98]=>
  int(99)
  [99]=>
  int(100)
}
array(100) {
  [0]=>
  int(100)
  [1]=>
  int(99)
  [2]=>
  int(98)
  [3]=>
  int(97)
  [4]=>
  int(96)
  [5]=>
  int(95)
  [6]=>
  int(94)
  [7]=>
  int(93)
  [8]=>
  int(92)
  [9]=>
  int(91)
  [10]=>
  int(90)
  [11]=>
  int(89)
  [12]=>
  int(88)
  [13]=>
  int(87)
  [14]=>
  int(86)
  [15]=>
  int(85)
  [16]=>
  int(84)
  [17]=>
  int(83)
  [18]=>
  int(82)
  [19]=>
  int(81)
  [20]=>
  int(80)
  [21]=>
  int(79)
  [22]=>
  int(78)
  [23]=>
  int(77)
  [24]=>
  int(76)
  [25]=>
  int(75)
  [26]=>
  int(74)
  [27]=>
  int(73)
  [28]=>
  int(72)
  [29]=>
  int(71)
  [30]=>
  int(70)
  [31]=>
  int(69)
  [32]=>
  int(68)
  [33]=>
  int(67)
  [34]=>
  int(66)
  [35]=>
  int(65)
  [36]=>
  int(64)
  [37]=>
  int(63)
  [38]=>
  int(62)
  [39]=>
  int(61)
  [40]=>
  int(60)
  [41]=>
  int(59)
  [42]=>
  int(58)
  [43]=>
  int(57)
  [44]=>
  int(56)
  [45]=>
  int(55)
  [46]=>
  int(54)
  [47]=>
  int(53)
  [48]=>
  int(52)
  [49]=>
  int(51)
  [50]=>
  int(50)
  [51]=>
  int(49)
  [52]=>
  int(48)
  [53]=>
  int(47)
  [54]=>
  int(46)
  [55]=>
  int(45)
  [56]=>
  int(44)
  [57]=>
  int(43)
  [58]=>
  int(42)
  [59]=>
  int(41)
  [60]=>
  int(40)
  [61]=>
  int(39)
  [62]=>
  int(38)
  [63]=>
  int(37)
  [64]=>
  int(36)
  [65]=>
  int(35)
  [66]=>
  int(34)
  [67]=>
  int(33)
  [68]=>
  int(32)
  [69]=>
  int(31)
  [70]=>
  int(30)
  [71]=>
  int(29)
  [72]=>
  int(28)
  [73]=>
  int(27)
  [74]=>
  int(26)
  [75]=>
  int(25)
  [76]=>
  int(24)
  [77]=>
  int(23)
  [78]=>
  int(22)
  [79]=>
  int(21)
  [80]=>
  int(20)
  [81]=>
  int(19)
  [82]=>
  int(18)
  [83]=>
  int(17)
  [84]=>
  int(16)
  [85]=>
  int(15)
  [86]=>
  int(14)
  [87]=>
  int(13)
  [88]=>
  int(12)
  [89]=>
  int(11)
  [90]=>
  int(10)
  [91]=>
  int(9)
  [92]=>
  int(8)
  [93]=>
  int(7)
  [94]=>
  int(6)
  [95]=>
  int(5)
  [96]=>
  int(4)
  [97]=>
  int(3)
  [98]=>
  int(2)
  [99]=>
  int(1)
}
array(100) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
  [5]=>
  int(6)
  [6]=>
  int(7)
  [7]=>
  int(8)
  [8]=>
  int(9)
  [9]=>
  int(10)
  [10]=>
  int(11)
  [11]=>
  int(12)
  [12]=>
  int(13)
  [13]=>
  int(14)
  [14]=>
  int(15)
  [15]=>
  int(16)
  [16]=>
  int(17)
  [17]=>
  int(18)
  [18]=>
  int(19)
  [19]=>
  int(20)
  [20]=>
  int(21)
  [21]=>
  int(22)
  [22]=>
  int(23)
  [23]=>
  int(24)
  [24]=>
  int(25)
  [25]=>
  int(26)
  [26]=>
  int(27)
  [27]=>
  int(28)
  [28]=>
  int(29)
  [29]=>
  int(30)
  [30]=>
  int(31)
  [31]=>
  int(32)
  [32]=>
  int(33)
  [33]=>
  int(34)
  [34]=>
  int(35)
  [35]=>
  int(36)
  [36]=>
  int(37)
  [37]=>
  int(38)
  [38]=>
  int(39)
  [39]=>
  int(40)
  [40]=>
  int(41)
  [41]=>
  int(42)
  [42]=>
  int(43)
  [43]=>
  int(44)
  [44]=>
  int(45)
  [45]=>
  int(46)
  [46]=>
  int(47)
  [47]=>
  int(48)
  [48]=>
  int(49)
  [49]=>
  int(50)
  [50]=>
  int(51)
  [51]=>
  int(52)
  [52]=>
  int(53)
  [53]=>
  int(54)
  [54]=>
  int(55)
  [55]=>
  int(56)
  [56]=>
  int(57)
  [57]=>
  int(58)
  [58]=>
  int(59)
  [59]=>
  int(60)
  [60]=>
  int(61)
  [61]=>
  int(62)
  [62]=>
  int(63)
  [63]=>
  int(64)
  [64]=>
  int(65)
  [65]=>
  int(66)
  [66]=>
  int(67)
  [67]=>
  int(68)
  [68]=>
  int(69)
  [69]=>
  int(70)
  [70]=>
  int(71)
  [71]=>
  int(72)
  [72]=>
  int(73)
  [73]=>
  int(74)
  [74]=>
  int(75)
  [75]=>
  int(76)
  [76]=>
  int(77)
  [77]=>
  int(78)
  [78]=>
  int(79)
  [79]=>
  int(80)
  [80]=>
  int(81)
  [81]=>
  int(82)
  [82]=>
  int(83)
  [83]=>
  int(84)
  [84]=>
  int(85)
  [85]=>
  int(86)
  [86]=>
  int(87)
  [87]=>
  int(88)
  [88]=>
  int(89)
  [89]=>
  int(90)
  [90]=>
  int(91)
  [91]=>
  int(92)
  [92]=>
  int(93)
  [93]=>
  int(94)
  [94]=>
  int(95)
  [95]=>
  int(96)
  [96]=>
  int(97)
  [97]=>
  int(98)
  [98]=>
  int(99)
  [99]=>
  int(100)
}
array(100) {
  [0]=>
  int(100)
  [1]=>
  int(99)
  [2]=>
  int(98)
  [3]=>
  int(97)
  [4]=>
  int(96)
  [5]=>
  int(95)
  [6]=>
  int(94)
  [7]=>
  int(93)
  [8]=>
  int(92)
  [9]=>
  int(91)
  [10]=>
  int(90)
  [11]=>
  int(89)
  [12]=>
  int(88)
  [13]=>
  int(87)
  [14]=>
  int(86)
  [15]=>
  int(85)
  [16]=>
  int(84)
  [17]=>
  int(83)
  [18]=>
  int(82)
  [19]=>
  int(81)
  [20]=>
  int(80)
  [21]=>
  int(79)
  [22]=>
  int(78)
  [23]=>
  int(77)
  [24]=>
  int(76)
  [25]=>
  int(75)
  [26]=>
  int(74)
  [27]=>
  int(73)
  [28]=>
  int(72)
  [29]=>
  int(71)
  [30]=>
  int(70)
  [31]=>
  int(69)
  [32]=>
  int(68)
  [33]=>
  int(67)
  [34]=>
  int(66)
  [35]=>
  int(65)
  [36]=>
  int(64)
  [37]=>
  int(63)
  [38]=>
  int(62)
  [39]=>
  int(61)
  [40]=>
  int(60)
  [41]=>
  int(59)
  [42]=>
  int(58)
  [43]=>
  int(57)
  [44]=>
  int(56)
  [45]=>
  int(55)
  [46]=>
  int(54)
  [47]=>
  int(53)
  [48]=>
  int(52)
  [49]=>
  int(51)
  [50]=>
  int(50)
  [51]=>
  int(49)
  [52]=>
  int(48)
  [53]=>
  int(47)
  [54]=>
  int(46)
  [55]=>
  int(45)
  [56]=>
  int(44)
  [57]=>
  int(43)
  [58]=>
  int(42)
  [59]=>
  int(41)
  [60]=>
  int(40)
  [61]=>
  int(39)
  [62]=>
  int(38)
  [63]=>
  int(37)
  [64]=>
  int(36)
  [65]=>
  int(35)
  [66]=>
  int(34)
  [67]=>
  int(33)
  [68]=>
  int(32)
  [69]=>
  int(31)
  [70]=>
  int(30)
  [71]=>
  int(29)
  [72]=>
  int(28)
  [73]=>
  int(27)
  [74]=>
  int(26)
  [75]=>
  int(25)
  [76]=>
  int(24)
  [77]=>
  int(23)
  [78]=>
  int(22)
  [79]=>
  int(21)
  [80]=>
  int(20)
  [81]=>
  int(19)
  [82]=>
  int(18)
  [83]=>
  int(17)
  [84]=>
  int(16)
  [85]=>
  int(15)
  [86]=>
  int(14)
  [87]=>
  int(13)
  [88]=>
  int(12)
  [89]=>
  int(11)
  [90]=>
  int(10)
  [91]=>
  int(9)
  [92]=>
  int(8)
  [93]=>
  int(7)
  [94]=>
  int(6)
  [95]=>
  int(5)
  [96]=>
  int(4)
  [97]=>
  int(3)
  [98]=>
  int(2)
  [99]=>
  int(1)
}
array(26) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
  [2]=>
  string(1) "c"
  [3]=>
  string(1) "d"
  [4]=>
  string(1) "e"
  [5]=>
  string(1) "f"
  [6]=>
  string(1) "g"
  [7]=>
  string(1) "h"
  [8]=>
  string(1) "i"
  [9]=>
  string(1) "j"
  [10]=>
  string(1) "k"
  [11]=>
  string(1) "l"
  [12]=>
  string(1) "m"
  [13]=>
  string(1) "n"
  [14]=>
  string(1) "o"
  [15]=>
  string(1) "p"
  [16]=>
  string(1) "q"
  [17]=>
  string(1) "r"
  [18]=>
  string(1) "s"
  [19]=>
  string(1) "t"
  [20]=>
  string(1) "u"
  [21]=>
  string(1) "v"
  [22]=>
  string(1) "w"
  [23]=>
  string(1) "x"
  [24]=>
  string(1) "y"
  [25]=>
  string(1) "z"
}
array(26) {
  [0]=>
  string(1) "z"
  [1]=>
  string(1) "y"
  [2]=>
  string(1) "x"
  [3]=>
  string(1) "w"
  [4]=>
  string(1) "v"
  [5]=>
  string(1) "u"
  [6]=>
  string(1) "t"
  [7]=>
  string(1) "s"
  [8]=>
  string(1) "r"
  [9]=>
  string(1) "q"
  [10]=>
  string(1) "p"
  [11]=>
  string(1) "o"
  [12]=>
  string(1) "n"
  [13]=>
  string(1) "m"
  [14]=>
  string(1) "l"
  [15]=>
  string(1) "k"
  [16]=>
  string(1) "j"
  [17]=>
  string(1) "i"
  [18]=>
  string(1) "h"
  [19]=>
  string(1) "g"
  [20]=>
  string(1) "f"
  [21]=>
  string(1) "e"
  [22]=>
  string(1) "d"
  [23]=>
  string(1) "c"
  [24]=>
  string(1) "b"
  [25]=>
  string(1) "a"
}
array(1) {
  [0]=>
  string(1) "q"
}
array(1) {
  [0]=>
  int(5)
}
array(6) {
  [0]=>
  float(5.1)
  [1]=>
  float(6.1)
  [2]=>
  float(7.1)
  [3]=>
  float(8.1)
  [4]=>
  float(9.1)
  [5]=>
  float(10.1)
}
array(6) {
  [0]=>
  float(10.1)
  [1]=>
  float(9.1)
  [2]=>
  float(8.1)
  [3]=>
  float(7.1)
  [4]=>
  float(6.1)
  [5]=>
  float(5.1)
}
array(6) {
  [0]=>
  float(5.1)
  [1]=>
  float(6.1)
  [2]=>
  float(7.1)
  [3]=>
  float(8.1)
  [4]=>
  float(9.1)
  [5]=>
  float(10.1)
}
array(6) {
  [0]=>
  float(10.1)
  [1]=>
  float(9.1)
  [2]=>
  float(8.1)
  [3]=>
  float(7.1)
  [4]=>
  float(6.1)
  [5]=>
  float(5.1)
}
array(41) {
  [0]=>
  float(1)
  [1]=>
  float(1.1)
  [2]=>
  float(1.2)
  [3]=>
  float(1.3)
  [4]=>
  float(1.4)
  [5]=>
  float(1.5)
  [6]=>
  float(1.6)
  [7]=>
  float(1.7)
  [8]=>
  float(1.8)
  [9]=>
  float(1.9)
  [10]=>
  float(2)
  [11]=>
  float(2.1)
  [12]=>
  float(2.2)
  [13]=>
  float(2.3)
  [14]=>
  float(2.4)
  [15]=>
  float(2.5)
  [16]=>
  float(2.6)
  [17]=>
  float(2.7)
  [18]=>
  float(2.8)
  [19]=>
  float(2.9)
  [20]=>
  float(3)
  [21]=>
  float(3.1)
  [22]=>
  float(3.2)
  [23]=>
  float(3.3)
  [24]=>
  float(3.4)
  [25]=>
  float(3.5)
  [26]=>
  float(3.6)
  [27]=>
  float(3.7)
  [28]=>
  float(3.8)
  [29]=>
  float(3.9)
  [30]=>
  float(4)
  [31]=>
  float(4.1)
  [32]=>
  float(4.2)
  [33]=>
  float(4.3)
  [34]=>
  float(4.4)
  [35]=>
  float(4.5)
  [36]=>
  float(4.6)
  [37]=>
  float(4.7)
  [38]=>
  float(4.8)
  [39]=>
  float(4.9)
  [40]=>
  float(5)
}
array(41) {
  [0]=>
  float(5)
  [1]=>
  float(4.9)
  [2]=>
  float(4.8)
  [3]=>
  float(4.7)
  [4]=>
  float(4.6)
  [5]=>
  float(4.5)
  [6]=>
  float(4.4)
  [7]=>
  float(4.3)
  [8]=>
  float(4.2)
  [9]=>
  float(4.1)
  [10]=>
  float(4)
  [11]=>
  float(3.9)
  [12]=>
  float(3.8)
  [13]=>
  float(3.7)
  [14]=>
  float(3.6)
  [15]=>
  float(3.5)
  [16]=>
  float(3.4)
  [17]=>
  float(3.3)
  [18]=>
  float(3.2)
  [19]=>
  float(3.1)
  [20]=>
  float(3)
  [21]=>
  float(2.9)
  [22]=>
  float(2.8)
  [23]=>
  float(2.7)
  [24]=>
  float(2.6)
  [25]=>
  float(2.5)
  [26]=>
  float(2.4)
  [27]=>
  float(2.3)
  [28]=>
  float(2.2)
  [29]=>
  float(2.1)
  [30]=>
  float(2)
  [31]=>
  float(1.9)
  [32]=>
  float(1.8)
  [33]=>
  float(1.7)
  [34]=>
  float(1.6)
  [35]=>
  float(1.5)
  [36]=>
  float(1.4)
  [37]=>
  float(1.3)
  [38]=>
  float(1.2)
  [39]=>
  float(1.1)
  [40]=>
  float(1)
}
array(41) {
  [0]=>
  float(1)
  [1]=>
  float(1.1)
  [2]=>
  float(1.2)
  [3]=>
  float(1.3)
  [4]=>
  float(1.4)
  [5]=>
  float(1.5)
  [6]=>
  float(1.6)
  [7]=>
  float(1.7)
  [8]=>
  float(1.8)
  [9]=>
  float(1.9)
  [10]=>
  float(2)
  [11]=>
  float(2.1)
  [12]=>
  float(2.2)
  [13]=>
  float(2.3)
  [14]=>
  float(2.4)
  [15]=>
  float(2.5)
  [16]=>
  float(2.6)
  [17]=>
  float(2.7)
  [18]=>
  float(2.8)
  [19]=>
  float(2.9)
  [20]=>
  float(3)
  [21]=>
  float(3.1)
  [22]=>
  float(3.2)
  [23]=>
  float(3.3)
  [24]=>
  float(3.4)
  [25]=>
  float(3.5)
  [26]=>
  float(3.6)
  [27]=>
  float(3.7)
  [28]=>
  float(3.8)
  [29]=>
  float(3.9)
  [30]=>
  float(4)
  [31]=>
  float(4.1)
  [32]=>
  float(4.2)
  [33]=>
  float(4.3)
  [34]=>
  float(4.4)
  [35]=>
  float(4.5)
  [36]=>
  float(4.6)
  [37]=>
  float(4.7)
  [38]=>
  float(4.8)
  [39]=>
  float(4.9)
  [40]=>
  float(5)
}
array(41) {
  [0]=>
  float(1)
  [1]=>
  float(1.1)
  [2]=>
  float(1.2)
  [3]=>
  float(1.3)
  [4]=>
  float(1.4)
  [5]=>
  float(1.5)
  [6]=>
  float(1.6)
  [7]=>
  float(1.7)
  [8]=>
  float(1.8)
  [9]=>
  float(1.9)
  [10]=>
  float(2)
  [11]=>
  float(2.1)
  [12]=>
  float(2.2)
  [13]=>
  float(2.3)
  [14]=>
  float(2.4)
  [15]=>
  float(2.5)
  [16]=>
  float(2.6)
  [17]=>
  float(2.7)
  [18]=>
  float(2.8)
  [19]=>
  float(2.9)
  [20]=>
  float(3)
  [21]=>
  float(3.1)
  [22]=>
  float(3.2)
  [23]=>
  float(3.3)
  [24]=>
  float(3.4)
  [25]=>
  float(3.5)
  [26]=>
  float(3.6)
  [27]=>
  float(3.7)
  [28]=>
  float(3.8)
  [29]=>
  float(3.9)
  [30]=>
  float(4)
  [31]=>
  float(4.1)
  [32]=>
  float(4.2)
  [33]=>
  float(4.3)
  [34]=>
  float(4.4)
  [35]=>
  float(4.5)
  [36]=>
  float(4.6)
  [37]=>
  float(4.7)
  [38]=>
  float(4.8)
  [39]=>
  float(4.9)
  [40]=>
  float(5)
}
