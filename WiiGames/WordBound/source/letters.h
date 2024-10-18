#ifndef LETTERS_H
#define LETTERS_H

#include <stdbool.h>
#include <string.h>

// Define the Letter structure
typedef struct {
    char name;      // Letter name
    int level;
    int status;
    char type;
    char type2;
    bool isVowel;
    int maxHP;
    int curHP;
    int maxAttack;
    int curAttack;
    int maxDefense;
    int curDefense;
    int exp;
    int expNeeded;
} Letter;

// Declare the userLetters variable as extern
extern Letter userLetters[5];  // Declare userLetters as an external array
extern Letter playerLetters[5];  // Declare other arrays as externs
extern Letter enemyLetters[5];
extern Letter playWord[5];
extern Letter enemyWord[5];

// Function prototypes
Letter InitializeLetter(char letterName, int level);
void createParty();

#endif // LETTERS_H


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
