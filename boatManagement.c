#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME_LEN 128



/* Enumeration for locations in Marina */ 
typedef enum { SLIP, LAND, TRAILOR, STORAGE } PlaceType; 


/* So I can print the names later */
const char* placeTypeNames[] = {"slip", "land", "trailor", "storage"};

/* Union for extra boat info */
typedef union {
    int slipNumber;         // For boats in slips (can be 1-85)
    char bayLetter;         // For boats on land (can be A-Z)
    char trailorTag[16];    // For boats on trailors (needs to have a license tag)
    int storageSpace;       // For boats in storage (can be 1-50)
} ExtraInfo;


/* Structure for boat */
typedef struct {
    char name[MAX_NAME_LEN]; // Boat name up to 127 character
    int length;              // Boat length up to 100'
    PlaceType type;          // Which type of location the boat is
    ExtraInfo extra;         // Extra data
    double amountOwed;       // Money owed for boat
} Boat;



/* Loading data from CSV file (regardless if one exists) */
void loadData(const char *filename, Boat *boats[], int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {  // File does not exist, start with new inventory
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';  // Remove newline
        if (strlen(line) == 0)
            continue;
        Boat *boat = malloc(sizeof(Boat));  //Allocate memory for boat
        if (!boat) {
            fprintf(stderr, "Memory allocation error.\n"); //Throw error if no boat exists
            exit(1);
        }
        /* CSV Format: name,length,type,extra,amountOwed */
        char *token = strtok(line, ",");  // Separate token
        if (!token) { free(boat); continue; } // If no token move on
        strncpy(boat->name, token, MAX_NAME_LEN); // Set boats name
        boat->name[MAX_NAME_LEN - 1] = '\0';  // Ensure boat name under 127 characters

	/* Separate next token, if one exists copy it into boats length */ 
        token = strtok(NULL, ","); 
        if (!token) { free(boat); continue; } 
        boat->length = atoi(token); 

	/* Separate next token, if one exists check which PlaceType it matches, and copy it */
        token = strtok(NULL, ",");
        if (!token) { free(boat); continue; }
        if (strcasecmp(token, "slip") == 0)
            boat->type = SLIP;
        else if (strcasecmp(token, "land") == 0)
            boat->type = LAND;
        else if (strcasecmp(token, "trailor") == 0)
            boat->type = TRAILOR;
        else if (strcasecmp(token, "storage") == 0)
            boat->type = STORAGE;
        else {
            free(boat);
            continue;
        }


	/* Depending on boat type set extra information if given */
        token = strtok(NULL, ",");
        if (!token) { free(boat); continue; }
        switch (boat->type) {
            case SLIP:
                boat->extra.slipNumber = atoi(token);
                break;
            case LAND:
                boat->extra.bayLetter = token[0];
                break;
            case TRAILOR:
                strncpy(boat->extra.trailorTag, token, sizeof(boat->extra.trailorTag));
                boat->extra.trailorTag[sizeof(boat->extra.trailorTag)-1] = '\0';
                break;
            case STORAGE:
                boat->extra.storageSpace = atoi(token);
                break;
        }

	/* Set amount owed if token exists, otherwise free boat and move on */
        token = strtok(NULL, ",");
        if (!token) { free(boat); continue; }
        boat->amountOwed = atof(token);

	/* Check to make sure boat limit is not reached */
        boats[*count] = boat;
        (*count)++;
        if (*count >= MAX_BOATS)
            break;
    }
    fclose(fp);
}

/* Save the data to CSV in correct format */
void saveData(const char *filename, Boat *boats[], int count) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing.\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        Boat *boat = boats[i];
        /* CSV Format: name,length,placeType,extra,amountOwed */
        switch (boat->type) {
            case SLIP:
                fprintf(fp, "%s,%d,slip,%d,%.2f\n", boat->name, boat->length,
                        boat->extra.slipNumber, boat->amountOwed);
                break;
            case LAND:
                fprintf(fp, "%s,%d,land,%c,%.2f\n", boat->name, boat->length,
                        boat->extra.bayLetter, boat->amountOwed);
                break;
            case TRAILOR:
                fprintf(fp, "%s,%d,trailor,%s,%.2f\n", boat->name, boat->length,
                        boat->extra.trailorTag, boat->amountOwed);
                break;
            case STORAGE:
                fprintf(fp, "%s,%d,storage,%d,%.2f\n", boat->name, boat->length,
                        boat->extra.storageSpace, boat->amountOwed);
                break;
        }
    }
    fclose(fp);
}

/* Case insensitive comparison function needed for quick sort */
int compareBoats(const void *a, const void *b) {
    Boat *const *boatA = (Boat *const *) a;
    Boat *const *boatB = (Boat *const *) b;
    return strcasecmp((*boatA)->name, (*boatB)->name);
}



/* Prints inventory of all boats */
void printInventory(Boat *boats[], int count) {
    for (int i = 0; i < count; i++) {
        Boat *boat = boats[i];
        printf("%-20s %2d' ", boat->name, boat->length);
        switch (boat->type) {
            case SLIP:
                printf("   slip   # %d", boat->extra.slipNumber);
                break;
            case LAND:
                printf("   land      %c", boat->extra.bayLetter);
                break;
            case TRAILOR:
                printf(" trailor %s", boat->extra.trailorTag);
                break;
            case STORAGE:
                printf(" storage   # %d", boat->extra.storageSpace);
                break;
        }
        printf("   Owes $%8.2f\n", boat->amountOwed);
    }
}


/* Gets index of boat */
int findBoatIndex(Boat *boats[], int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcasecmp(boats[i]->name, name) == 0)
            return i;
    }
    return -1;
}



/* Parse boat from single line of CSV formatted info (for use in adding boats) */
Boat *parseBoatFromCSV(const char *csvLine) {
    Boat *boat = malloc(sizeof(Boat));
    if (!boat) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(1);
    }
    char lineCopy[256];
    strncpy(lineCopy, csvLine, sizeof(lineCopy));
    lineCopy[sizeof(lineCopy)-1] = '\0';
    char *token = strtok(lineCopy, ",");
    if (!token) { free(boat); return NULL; }
    strncpy(boat->name, token, MAX_NAME_LEN);
    boat->name[MAX_NAME_LEN-1] = '\0';

    token = strtok(NULL, ",");
    if (!token) { free(boat); return NULL; }
    boat->length = atoi(token);

    token = strtok(NULL, ",");
    if (!token) { free(boat); return NULL; }
    if (strcasecmp(token, "slip") == 0)
        boat->type = SLIP;
    else if (strcasecmp(token, "land") == 0)
        boat->type = LAND;
    else if (strcasecmp(token, "trailor") == 0)
        boat->type = TRAILOR;
    else if (strcasecmp(token, "storage") == 0)
        boat->type = STORAGE;
    else { free(boat); return NULL; }

    token = strtok(NULL, ",");
    if (!token) { free(boat); return NULL; }
    switch (boat->type) {
        case SLIP:
            boat->extra.slipNumber = atoi(token);
            break;
        case LAND:
            boat->extra.bayLetter = token[0];
            break;
        case TRAILOR:
            strncpy(boat->extra.trailorTag, token, sizeof(boat->extra.trailorTag));
            boat->extra.trailorTag[sizeof(boat->extra.trailorTag)-1] = '\0';
            break;
        case STORAGE:
            boat->extra.storageSpace = atoi(token);
            break;
    }

    token = strtok(NULL, ",");
    if (!token) { free(boat); return NULL; }
    boat->amountOwed = atof(token);

    return boat;
}



/* Add boat given in CSV formatted string by user */
void addBoat(Boat *boats[], int *count, const char *csvLine) {
    if (*count >= MAX_BOATS) {
        printf("Marina is full. Cannot add more boats.\n");
        return;
    }
    /* Create boat struct from string and check validity */
    Boat *newBoat = parseBoatFromCSV(csvLine);
    if (newBoat == NULL) {
        printf("Invalid boat data.\n");
        return;
    }
    /* Insert in sorted order by boat name */
    int i = *count - 1;
    while (i >= 0 && strcasecmp(boats[i]->name, newBoat->name) > 0) {
        boats[i + 1] = boats[i];
        i--;
    }
    boats[i + 1] = newBoat;
    (*count)++;
}


/* Remove a boat if such exists */
void removeBoat(Boat *boats[], int *count, const char *name) {
    int index = findBoatIndex(boats, *count, name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[index]);
    for (int i = index; i < (*count) - 1; i++) {
        boats[i] = boats[i + 1];
    }
    (*count)--;
}


/* Take payment from user */
void acceptPayment(Boat *boats[], int count, const char *name) {
    int index = findBoatIndex(boats, count, name);
    /* Check if boat exists */
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    Boat *boat = boats[index];
    char input[64];
    printf("Please enter the amount to be paid                       : ");
    if (fgets(input, sizeof(input), stdin) == NULL)
        return;
    double payment = atof(input);
    /* Ensure payment does not exceed amount owed, if so do not accept */
    if (payment > boat->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", boat->amountOwed);
        return;
    }
    boat->amountOwed -= payment;
}



/* Calculates amount owed at new Month based on rate and length, update boat */
void updateMonth(Boat *boats[], int count) {
    for (int i = 0; i < count; i++) {
        Boat *boat = boats[i];
        double rate = 0.0;
        switch (boat->type) {
            case SLIP:    rate = 12.50; break;
            case LAND:    rate = 14.00; break;
            case TRAILOR: rate = 25.00; break;
            case STORAGE: rate = 11.20; break;
        }
        boat->amountOwed += boat->length * rate;
    }
}


/* Main method for program, print menu and accept options */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    Boat *boats[MAX_BOATS];
    int count = 0;

    /* Load data from file */
    loadData(filename, boats, &count);
    /* Sort the array alphabetically by boat name */
    qsort(boats, count, sizeof(Boat *), compareBoats);

    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n");
    char option[10];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        if (fgets(option, sizeof(option), stdin) == NULL)
            break;
        option[strcspn(option, "\n")] = '\0';
        if (strlen(option) == 0)
            continue;
        char choice = toupper(option[0]);
        switch (choice) {
            case 'I':
                printInventory(boats, count);
                break;
            case 'A': {
                char csvLine[256];
                printf("Please enter the boat data in CSV format                 : ");
                if (fgets(csvLine, sizeof(csvLine), stdin) == NULL)
                    break;
                csvLine[strcspn(csvLine, "\n")] = '\0';
                addBoat(boats, &count, csvLine);
                break;
            }
            case 'R': {
                char nameInput[128];
                printf("Please enter the boat name                               : ");
                if (fgets(nameInput, sizeof(nameInput), stdin) == NULL)
                    break;
                nameInput[strcspn(nameInput, "\n")] = '\0';
                removeBoat(boats, &count, nameInput);
                break;
            }
            case 'P': {
                char nameInput[128];
                printf("Please enter the boat name                               : ");
                if (fgets(nameInput, sizeof(nameInput), stdin) == NULL)
                    break;
                nameInput[strcspn(nameInput, "\n")] = '\0';
                acceptPayment(boats, count, nameInput);
                break;
            }
            case 'M':
                updateMonth(boats, count);
                break;
            case 'X':
                goto exit_loop;
            default:
                printf("Invalid option %s\n", option);
                break;
        }
}

    /* Exit Program and save the data */
exit_loop:
    printf("\nExiting the Boat Management System\n");
    /* Save updated data back to the CSV file */
    saveData(filename, boats, count);
    for (int i = 0; i < count; i++) {
        free(boats[i]);
    }
    return 0;
}
