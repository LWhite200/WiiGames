#include "letters.h"

// Function to initialize a letter with stats based on its name and level
Letter InitializeLetter(char letterName, int level) {
    Letter letter;
    letter.name = letterName;
    letter.level = level;

    /*


            collumns (Balanced,Offensive,Defensive,Crazy,)  crazy = can be random like avoid attacks, be stronger, survive an attack that should've killed

            a - static (for 2 turns) if no vowel --- poisen if vowel at end of word
            e - evasion (current turn) --- 3 turns if vowel at end
            i - intelligence (next attack is 2x damage) --- allows for critical hits
            o - Defense, boosts its team's defense for 3 turns --- boosts its team defense and avoids critical hits in sacrific
            u - umbrella (removes status) -- removes status and blocks it for next 2 turns

            Types
            a,b,c,d (Grey)

            e,f,g,h (Blue)
            i,j,k,l (Red)
            m,n,o,p (Green)
            q,r,s,t (Brown)
            u,v,w,x (Yellow)
            y,z     (pink)

            1,2     (Purple)
            3,4,5,6 (Lime)
            7,8,9   (Orange)
            !,?     (Banana)


            // e,b,r,g,w,y,p,l,m,o,a --- grey,blue,red,green,brown,yellow,pink,purple,lime,orange,banana


*/
    letter.type2 = 'Y';

    // Set stats based on the letter type and level
    switch (letterName) {
    case 'A':
        letter.isVowel = true;
        letter.type = 'E';
        letter.type2 = 'B';
        break;
    case 'B':
        letter.isVowel = false;
        letter.type = 'E';
        letter.type2 = 'R';
        break;
    case 'C':
        letter.isVowel = false;
        letter.type = 'E';
        letter.type2 = 'G';
        break;
    case 'D':
        letter.isVowel = false;
        letter.type = 'E';
        letter.type2 = 'W';
        break;
    case 'E':
        letter.isVowel = true;
        letter.type = 'B';
        letter.type2 = 'Y';
        break;
    case 'F':
        letter.isVowel = false;
        letter.type = 'B';
        letter.type2 = 'P';
        break;
    case 'G':
        letter.isVowel = false;
        letter.type = 'B';
        letter.type2 = 'L';
        break;
    case 'H':
        letter.isVowel = false;
        letter.type = 'B';
        letter.type2 = 'M';
        break;
    case 'I':
        letter.isVowel = true;
        letter.type = 'R';
        letter.type2 = 'O';
        break;
    case 'J':
        letter.isVowel = false;
        letter.type = 'R';
        letter.type2 = 'A';
        break;
    case 'K':
        letter.isVowel = false;
        letter.type = 'R';
        break;
    case 'L':
        letter.isVowel = false;
        letter.type = 'R';
        break;
    case 'M':
        letter.isVowel = false;
        letter.type = 'G';
        break;
    case 'N':
        letter.isVowel = false;
        letter.type = 'G';
        break;
    case 'O':
        letter.isVowel = true;
        letter.type = 'G';
        break;
    case 'P':
        letter.isVowel = false;
        letter.type = 'G';
        break;
    case 'Q':
        letter.isVowel = false;
        letter.type = 'W';
        break;
    case 'R':
        letter.isVowel = false;
        letter.type = 'W';
        break;
    case 'S':
        letter.isVowel = false;
        letter.type = 'W';
        break;
    case 'T':
        letter.isVowel = false;
        letter.type = 'W';
        break;
    case 'U':
        letter.isVowel = false;
        letter.type = 'Y';
        break;
    case 'V':
        letter.isVowel = false;
        letter.type = 'Y';
        break;
    case 'W':
        letter.isVowel = false;
        letter.type = 'Y';
        break;
    case 'X':
        letter.isVowel = false;
        letter.type = 'Y';
        break;
    case 'Y':
        letter.isVowel = false;
        letter.type = 'P';
        break;
    case 'Z':
        letter.isVowel = false;
        letter.type = 'P';
        break;




        // Add more cases for other letters

        // Add more cases for other letters

    default:
        letter.isVowel = false;
        letter.type = 'Y';
        break;
    }

    
    letter.maxHP = 400;
    letter.curHP = letter.maxHP;
    letter.maxAttack = 100;
    letter.curAttack = letter.maxAttack;
    letter.maxDefense = 100;
    letter.curDefense = letter.maxDefense;


    // Example experience logic
    letter.exp = 0;
    letter.expNeeded = 100 + level * 20;

    return letter;
}
