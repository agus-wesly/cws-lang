# CWS Programming Language

CWS is a toy scripting programming language created for learning purposes. CWS is inspired by python, javascript and has an Indonesian like syntax with the interpreter is written in C. The project is still under development, the language syntax is not stable and bug is to be expected.

This language is based on what i learned from : https://craftinginterpreters.com

## Building the interpreter

```
make
```

## Compiling

Create a `.ws` file and use the interpreter to execute it

```
./cws hello.ws
```

You can also try the online playground here at : https://agus-wesly.github.io/cws-lang

# Guide

## The Basics
### Constants and Variables
The value of a constant can't be changed while a variable can be set to a different value in the future.

### Declaring Constant and Variables
```
konst maksPercobaan = 10; // Constant
andai percobaan = 0; // Variables
```

### Printing Constants and Variables
```
andai salam = "Hallo, Dunia !";
tampil(salam);
```
Include name with constant or variable
```
andai nama = "Wesly";
tampil("Halo, nama saya " + nama);
```

### Comments
```
// Ini adalah komentar
```

### Semicolons
CWS requires semicolon at the end of each statement
```
andai kucing = "🐱"; tampil(kucing);
```

### Number
Numbers are most commonly expressed in literal forms like 255 or 3.14159
```
andai integer = 255;
andai floatingPoint = 3.14159;
tampil(integer);
tampil(floatingPoint);

tampil(integer + floatingPoint);
```

### Boolean
CWS provides two boolean constant value, `sah`/`true` and `sesat`/`false`
```
andai jerukAdalahJeruk = sah; // true
andai sushiItuEnak = sesat; // false 

jika(sushiItuEnak) {
    tampil("Mmm, sushi enak banget");
} pula {
    tampil("Eww, sushi gaenak jir");
}
// Print Eww, sushi gaenak jir
```


## Nihil
You set an optional variable to a valueless state by assigning it the special value `nihil`:
```
andai responseServer = nihil;
jika (responseServer != nihil) {
    tampil(responseServer);
}
```

## Basic Operator
### Assignment Operator
```
andai b = 10;
andai a = 5;
a = b;
// a is now equal to 10
```
Assignment in CWS will return the value its set.
```
jika(a = b) { // Evaluate to b
    // This is valid
}
```

### Arithmetic Operators
CWS supports the four standard arithmetic operators for all number types:

1. Addition (+)
1. Subtraction (-)
1. Multiplication (*)
1. Division (/)

```
1 + 2;       // equals 3
5 - 3;      // equals 2
2 * 3;      // equals 6
10.0 / 2.5;  // equals 4.0
```
The addition operator is also supported for string concatenation:
```
"hello, " + "world";
```

### Unary Minus Operator
The sign of a numeric value can be toggled using a prefixed -, known as the unary minus operator:

```
andai three = 3;
andai minusThree = -three;       // minusThree equals -3
andai plusThree = -minusThree;   // plusThree equals 3, or minus three
```

### Comparison Operator
CWS supports the following comparison operators:
1. Equal to (a == b)
1. Not equal to (a != b)
1. Greater than (a > b)
1. Less than (a < b)
1. Greater than or equal to (a >= b)
1. Less than or equal to (a <= b)

```
1 == 1;   // true because 1 is equal to 1
2 != 1;  // true because 2 isn't equal to 1
2 > 1;    // true because 2 is greater than 1
1 < 2;    // true because 1 is less than 2
1 >= 1;   // true because 1 is greater than or equal to 1
2 <= 1;   // false because 2 isn't less than or equal to 1
```
Comparison operators are often used in conditional statements, such as the `jika` statement:
```
andai nama = "world";
jika (nama == "world") {
    tampil("hello, world");
}  pula {
    tampil("maaf, " + nama + " saya tidak mengenali anda");
}
// Prints "hello, world", because name is indeed equal to "world".
```
For more about the `jika` statement, see [Control Flow](https://github.com/agus-wesly/cws-lang)

Non Primitive data is compared based on their reference.
```
tampil([] == []); // false because different reference
tampil({} == {}); // false because different reference

andai array1 = [];
tampil (array1 == array1); //true because same reference
```
### Ternary Operator
The ternary conditional operator is a special operator with three parts, which takes the form question ? answer1 : answer2. It’s a shortcut for evaluating one of two expressions based on whether question is true or false. If question is true, it evaluates answer1 and returns its value; otherwise, it evaluates answer2 and returns its value.
```
andai jmlhPermen = 6;
andai keterangan = jmlhPermen > 5 ? "Lebih dari 5" : "Kurang dari 5";
// Lebih dari 5
tampil (keterangan);
```

### Logical Operator
Logical operators modify or combine the Boolean logic values `sah` and `sesat`. CWS supports the three standard logical operators found in C-based languages:
1. Logical NOT (!a)
1. Logical AND (a `dan` b)
1. Logical OR (a || b)

### Logical NOT Operator
The logical NOT operator (!a) inverts a Boolean value so that `sah` becomes `sesat`, and `sesat` becomes `sah`.

The logical NOT operator is a prefix operator, and appears immediately before the value it operates on, without any white space. It can be read as “not a”, as seen in the following example:
```
andai bolehMasuk = sesat;
jika (!bolehMasuk) {
    tampil("AKSES DILARANG");
}
// Prints "AKSES DILARANG"
```
As in this example, careful choice of Boolean constant and variable names can help to keep code readable and concise, while avoiding double negatives or confusing logic statements.


### Logical AND Operator
The logical AND operator (a `dan` b) creates logical expressions where both values must be truthy for the overall expression to also be true.

If either value is falsy, the overall expression will also be false. In fact, if the first value is falsy, the second value won’t even be evaluated, because it can’t possibly make the overall expression equate to true. This is known as short-circuit evaluation.

This example considers two Bool values and only allows access if both values are truthy:
```
andai kodePintuTerinput = sah;
andai hasilScanRetina = sesat;
jika (kodePintuTerinput dan hasilScanRetina) {
    tampil("Selamat Datang!");
} pula {
    tampil("AKSES DILARANG");
}
// Prints "ACCESS DILARANG"
```

### Logical OR Operator
The logical OR operator (a `atau` b) is an infix operator made from two adjacent pipe characters. You use it to create logical expressions in which only one of the two values has to be true for the overall expression to be true.

Like the Logical AND operator above, the Logical OR operator uses short-circuit evaluation to consider its expressions. If the left side of a Logical OR expression is true, the right side isn’t evaluated, because it can’t change the outcome of the overall expression.

In the example below, the first Bool value (kunciPintuTersedia) is false, but the second value (passwordDiketahui) is true. Because one value is true, the overall expression also evaluates to true, and access is allowed:

```
andai kunciPintuTersedia = sesat;
andai passwordDiketahui = sah;
jika (kunciPintuTersedia atau passwordDiketahui) {
    tampil("Selamat Datang!");
} pula {
    tampil("AKSES DILARANG");
}
// Print "Selamat Datang!"

```

### Combining Logical Operators
You can combine multiple logical operators to create longer compound expressions:
```
andai kodePintuTerinput = sah;
andai hasilScanRetina = sesat;
andai kunciPintuTersedia = sesat;
andai passwordDiketahui = sah;

jika (kodePintuTerinput dan hasilScanRetina atau kunciPintuTersedia atau passwordDiketahui) {
    tampil("Selamat Datang!");
} pula {
    tampil("AKSES DILARANG");
}
// Prints Selamat Datang!
```
### Explicit Parentheses 
It’s sometimes useful to include parentheses when they’re not strictly needed, to make the intention of a complex expression easier to read. In the door access example above, it’s useful to add parentheses around the first part of the compound expression to make its intent explicit:
```
jika ((kodePintuTerinput dan hasilScanRetina) atau (kunciPintuTersedia atau passwordDiketahui)) {
    tampil("Selamat Datang!");
} pula {
    tampil("AKSES DILARANG");
}
// Prints Selamat Datang!
```
The parentheses make it clear that the first two values are considered as part of a separate possible state in the overall logic. The output of the compound expression doesn’t change, but the overall intention is clearer to the reader. Readability is always preferred over brevity; use parentheses where they help to make your intentions clear.

## Strings
### String Literals
Use a string literal as an initial value for a constant or variable:
```
andai contohString = "Contoh suatu nilai string";
```
Find out whether a string value is empty by checking its length with `jmlh` built in fuction:
```
andai emptyString = "";
jika(jmlh(emptyString)) {
    tampil("Ada");
} pula {
    tampil("Tidak ada");
}
// Prints Tidak ada
```

### Concatenating String
String values can be added together (or concatenated) with the addition operator (+) to create a new String value:

```
konst string1 = "hallo";
konst string2 = " semuanya";
andai hallo = string1 + string2;
// hallo berisi "hallo semuanya"
```

### Counting Character
To retrieve a count of the character values in a string, use the `jmlh` built in function:
```
andai kumpulanBinatang = "Koala, Siput, Pinguin, Unta";
tampil ("kumpulanBinatang has " + jmlh(kumpulanBinatang) + " characters");
// Prints "kumpulanBinatang has 40 characters"
```
