# CWS Programming Language

CWS is a toy scripting programming language created for learning purposes. CWS is inspired by python, javascript and has an Indonesian like syntax with the interpreter is written in C. 

> [!WARNING]
> THIS LANGUAGE IS A WORK IN PROGRESS! THE PROJECT IS STILL UNDER DEVELOPMENT, THE LANGUAGE SYNTAX IS NOT STABLE AND BUG IS TO BE EXPECTED.


> [!NOTE]
> This language is based on what I learned from [this book](https://craftinginterpreters.com)

## Table of Contents
  - [The Basics](#the-basics)
    - [Constant and Variables](#constants-and-variables)
    - [Printing](#printing-constants-and-variables)
    - [Comments](#comments)
    - [Semicolons](#semicolons)
    - [Boolean](#boolean)
    - [Nihil](#nihil)
  - [Basic Operator](#basic-operator)
    - [Assignment Operator](#assignment-operator)
    - [Arithmetic Operator](#arithmetic-operator)
    - [Unary Minus Operator](#unary-minus-operator)
    - [Comparison Operator](#comparison-operator)
    - [Ternary Operator](#ternary-operator)
    - [Logical Operator](#logical-operator)
    - [Logical NOT Operator](#logical-not-operator)
    - [Logical AND Operator](#logical-and-operator)
    - [Combining Logical Operator](#combining-logical-operator)
    - [Explicit Parentheses](#explicit-parentheses)
    - [Strings](#strings)
        - [String Literals](#string-literals)
        - [Counting Character](#counting-character)
    - [Collection Types](#collection-types)
        - [Array](#array)
        - [Create Array](#create-array)
        - [Access and Modify Array](#access-and-modify-array)
        - [Iterating the Array](#iterating-the-array)
        - [Table](#table)
        - [Creating a Table](#creating-a-table)
        - [Accessing and Modifying Table](#accessing-and-modifying-table)
    - [Control Flow](#control-flow)
        - [For Loops](#for-loops)
        - [While Loops](#while-loops)
        - [Conditional Statements](#conditional-statements)
        - [If](#if)
        - [Switch](#switch)
        - [Control Flow Statement](#control-flow-statement)
            - [Continue](#continue)
            - [Break](#break)
        - [Function](#function)
        - [Defining and Calling Functions](#defining-and-calling-functions)
        - [Function without Parameters](#function-without-parameters)
        - [Function with Multiple Parameters](#function-with-multiple-parameters)
        - [Function with Return Values](#function-with-return-values)
        - [Function without Return Values](#function-without-return-values)


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


### Nihil
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
For more about the `jika` statement, see [Control Flow](#control-flow)

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
1. Logical OR (a `or` b)

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

## Collection Types
### Array
An array stores values in an ordered list. Array in CWS can store different type of value.The same value can appear in an array multiple times at different positions.

#### Create Array
```
andai kumpulanData1 = []; // Empty initialization
andai kumpulanData2 = [10, 69, "Hello", sah]; // Non-empty initialization
```

### Access and Modify Array
Use the `jmlh()` built-in function to get number of items in array.
```
andai kumpulanData2 = [10, 69, "Hello", sah]; // Non-empty initialization
tampil("kumpulanData2 berisi " + jmlh(kumpulanData2) +" items.");
```

You can add a new item to the end of an array by calling the array’s `push()` method:
```
andai kumpulanData2 = [10, 69, "Hello", sah]; // Non-empty initialization
kumpulanData2.push("Data baru");
tampil(kumpulanData2);
```

Retrieve a value from the array by using subscript syntax, passing the index of the value you want to retrieve within square brackets immediately after the name of the array:
```
andai daftarBelanja = ["Telur", "Susu"];
andai itemPertama = daftarBelanja[0]; // Prints "Telur"
```

You can also remove the last item in the array by calling array’s `pop()` method:
```
andai daftarBelanja = ["Telur", "Susu"];
daftarBelanja.pop();
```
### Iterating the array
You can iterate over the entire set of values in an array with the for loop with(`ulang` keyword):
```
andai daftarBelanja = ["Telur", "Susu", "Garam"];
ulang(andai i=0; i<jmlh(daftarBelanja); i=i+1){
    tampil(daftarBelanja[i]);
}
```
### Table
A table stores associations between keys of the type string and values in a collection with no defined ordering. Each value is associated with a unique key, which acts as an identifier for that value within the table. 
### Creating a table
```
andai hewanKebunBinatang = {
    "singa": 8,
    "zebra": 16,
    "rusa": 19,
    "jerapah": 5,
};
```

### Accessing and modifying table
As with an array, you find out the number of items in a table by using `jmlh()` function:
```
andai hewanKebunBinatang = {
    "singa": 8,
    "zebra": 16,
    "rusa": 19,
    "jerapah": 5,
};
jmlh(hewanKebunBinatang);
```
Get and Set the value in Table
```
andai hewanKebunBinatang = {
    "singa": 8,
    "zebra": 16,
    "rusa": 19,
    "jerapah": 5,
};

tampil("Jumlah singa ada " + hewanKebunBinatang.singa); // get value
hewanKebunBinatang["singa"] = 10; // set value
tampil("Jumlah singa sekarang ada " + hewanKebunBinatang.singa);
```

## Control Flow
### For Loops
For loops is done using `ulang` keyword
```
ulang(andai i=0; i<10; i=i+1) {
    tampil(i);
}
```

### While Loops
A while loop performs a set of statements until a condition becomes false. Use the `saat` keyword to perform it.
```
andai mengulang = sah;
andai jumlahUlang = 0;
saat(mengulang == sah) {
    jika (jumlahUlang < 5){
        tampil("Sedang mengulang ke - " + jumlahUlang);
        jumlahUlang=jumlahUlang+1;
    } pula {
        mengulang = sesat;
    }
}
```

### Conditional Statements
It’s often useful to execute different pieces of code based on certain conditions. You might want to run an extra piece of code when an error occurs, or to display a message when a value becomes too high or too low. To do this, you make parts of your code conditional.
### If
In its simplest form, the if statement has a single if condition. It executes a set of statements only if that condition is true. Use the `jika` keyword to implement it.
```
andai temperatureDalamFahrenheit = 30;
jika (temperatureDalamFahrenheit <= 32) {
    tampil("Diluar sangat dingin. Disarankan memakai syal");
}
// Print "Diluar sangat dingin. Disarankan memakai syal"
```
The if statement can provide an alternative set of statements, known as an else clause, for situations when the if condition is false. These statements are indicated by the `pula` keyword.
```
andai temperatureDalamFahrenheit = 40;
jika (temperatureDalamFahrenheit <= 32) {
    tampil("Diluar sangat dingin. Disarankan memakai syal");
} pula {
    tampil("Tidak terlalu dingin. Sebaiknya gunakan kaos");
}
```
You can chain multiple if statements together to consider additional clauses.
```
andai temperatureDalamFahrenheit = 90;
jika (temperatureDalamFahrenheit <= 32) {
    tampil("Diluar sangat dingin. Disarankan memakai syal");
} pula jika(temperatureDalamFahrenheit >=86)  {
    tampil("Diluar sangat panas. Disarankan menggunakan sunscreen");
} pula {
    tampil("Tidak terlalu dingin. Sebaiknya gunakan kaos");
}
```

### Switch
A switch statement considers a value and compares it against several possible matching patterns. It then executes an appropriate block of code, based on the first pattern that matches successfully. Implement it using the `kawal` keyword.
This example uses a switch statement to consider a single lowercase character called someCharacter:

```
andai suatuKarakter = "z";
kawal (suatuKarakter) {
hal "a":
    tampil("Huruf pertama dalam alfabet");
    kelar;
hal "z":
    tampil("Huruf terakhir dalam alfabet");
    kelar;
bawaan:
    tampil("Karakter lainnya");
    kelar;
}
```
Like in C, switch statement in CWS does fallthrough. Use the `kelar` keyword to finish the switch statement after a case is executed.

### Control Flow Statement
Control transfer statements change the order in which your code is executed, by transferring control from one piece of code to another. CWS has three control transfer statements:
1. `continue` / `lagi`
1. `break` / `kelar`
1. `return` / `balik`
The continue, break, and fallthrough statements are described below. The return statement is described in [Functions section](#function)

### Continue
The continue statement tells a loop to stop what it’s doing and start again at the beginning of the next iteration through the loop. To implement it use the `lagi` keyword.
```
ulang(andai i=0; i<10; i=i+1) {
    jika(i==5){
        lagi; // 5 will not printed
    } pula {
        tampil(i);
    }
}
```

### Break
The break statement ends execution of an entire control flow statement immediately. Implement it using the `kelar` keyword. You can also use it inside the switch statement to prevent the fallthrough.
```
// For loop
ulang(andai i=0; i<10; i=i+1) {
    jika(i==5){
        kelar;
    } pula {
        tampil(i);
    }
}

// Switch stmt
andai suatuKarakter = "z";
kawal (suatuKarakter) {
hal "a":
    tampil("Huruf pertama dalam alfabet");
    kelar;
hal "z":
    tampil("Huruf terakhir dalam alfabet");
    kelar;
bawaan:
    tampil("Karakter lainnya");
    kelar;
}
```

### Function
#### Defining and Calling Functions
Define a function by using the `fungsi` keyword.
```
fungsi sapa(orang) {
    andai sapaan = "Hallo, " + orang + "!";
    balik sapaan;
}

tampil(sapa("Anna"));
// Prints "Hallo, Anna!"
tampil(sapa("Brian"));
// Prints "Hallo, Brian!"
```

### Function without parameters
Functions aren’t required to define input parameters. Here’s a function with no input parameters, which always returns the same String message whenever it’s called:
```
fungsi haloDunia() {
    balik "hallo, dunia";
}
tampil(haloDunia());
// Prints "hallo, dunia"
```

### Functions with multiple parameters
Functions can have multiple input parameters, which are written within the function’s parentheses, separated by commas.

```
fungsi sum(x,y,z) {
    balik x + y + z;
}
```


### Functions with return values
Function can have return value. The return value is returned using the `balik` keyword
```
fungsi sum(x,y,z) {
    balik x + y + z;
}
```

### Functions without return values
Functions aren’t required to define a return type. If it does't have one, it will return `nihil` by default
```
fungsi sapa(person) {
    tampil("Hello " + person);
}
andai x = sapa("Dave");
tampil(x); // Nihil
```
