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

## Hello World

```
tampil("Halo, Dunia");

```

## Comment

```
// Ini adalah komentar
```

## Variable Declaration

```
// Deklarasi variable assignable
andai x = 69;

// Deklarasi variable konstan
konst y = 10;

x = 10;
x = "Something else";
```

## Branching

```
andai x = 10;
jika(x < 10){ // if
    tampil("Lebih kecil dari 10");
} pula jika (x == 10) { // else if
    tampil("Sama dengan 10");
} pula { // else
    tampil("Lebih besar dari 10");
}
```

## Looping

### For Loop

```
ulang(andai i=0; i<10; i=i+1){
    tampil(i);
}

```

### While Loop

```
andai i = 0;
saat(i<10){
    tampil(i);
    i=i+1;
}
```

## Data Type

CWS is a dynamic language meaning you can assign any variable into any type of data dynamically. There are 9 type of data : `number`, `string`, `boolean`, `nil`, `function`, `class`, `instance`, `array` and `table`.

## Number, String, Boolean and Nil

```
andai x = 58; // number
andai y = "11"; // string
tampil(x + y);

andai benar = sah; // boolean true
andai salah = sesat; // boolean false
jika (benar){
    tampil("Benar");
} pula {
    tampil("Salah");
}

andai kosong = nihil; // nil
jika(!kosong) {
    tampil("Nihil");
}
```

## Function

```
fungsi foo(x){
    tampil("Input berupa " + x);
    // Mengembalikan (return) nilai dari fungsi
    balik x;
}
konst balikan = foo(12);
tampil(balikan);
```

## Class and Instance

```
kelas Orang {
    init(nama, umur){
        // anu is like this in javascript.
        anu.nama = nama;
        anu.umur = umur;
    }
    sayHi(){
        tampil("Hi, Nama saya " + anu.nama + " dan umur saya " + anu.umur);
    }
}

andai fuad = Orang("Fuad", 69);
fuad.sayHi();
```

## Array

```
konst array = [10,2,3,1];
tampil(array);

andai sum = 0;
ulang(andai i=0; i<jmlh(array); i=i+1){ // Iterate the array
    sum = sum + array[i];
}
tampil("Totalnya adalah " + sum);

array.push(69);
tampil(array);

array.pop();
tampil(array);

```

## Table

```
konst obj = {"name": "Budi", "age": 69};
tampil(obj);

obj.name = "Andi"; //set key
tampil(obj);
tampil(obj.name); // get key

basmi obj.name; // delete key
tampil(obj);
```
