#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include "data.h"

#define MAX_USERS       100
#define MAX_DRIVERS     100
#define MAX_BOOKINGS    500
#define MAX_NAME        50
#define MAX_PHONE       15
#define MAX_PASS        30
#define MAX_VEHICLE     50
#define MAX_PLATE       20
#define DATA_FILE       "ride_data.dat"


typedef enum { PASSENGER = 1, DRIVER, ADMIN } UserRole;
typedef enum { AVAILABLE = 1, ON_TRIP, OFFLINE } DriverStatus;
typedef enum { PENDING = 1, ACCEPTED, IN_PROGRESS, COMPLETED, CANCELLED } BookingStatus;
typedef enum { ECONOMY = 1, COMFORT, XL } RideType;

typedef struct {
    int    id;
    char   name[MAX_NAME];
    char   phone[MAX_PHONE];
    char   password[MAX_PASS];
    UserRole role;
    float  balance;
    float  rating;
    int    totalRides;
} User;

typedef struct {
    int    id;
    char   name[MAX_NAME];
    char   phone[MAX_PHONE];
    char   password[MAX_PASS];
    char   vehicle[MAX_VEHICLE];
    char   plate[MAX_PLATE];
    DriverStatus status;
    float  rating;
    int    totalTrips;
    float  earnings;
    float  lat, lon;   /* simulated location */
} Driver;

typedef struct {
    int    id;
    int    userId;
    int    driverId;    /* -1 if not yet assigned */
    char   pickupLoc[60];
    char   dropLoc[60];
    float  pickupLat, pickupLon;
    float  dropLat,   dropLon;
    float  distance;   /* km */
    float  fare;
    RideType  rideType;
    BookingStatus status;
    char   createdAt[25];
    int    userRating;   /* 1-5, 0 = not rated */
    int    driverRating;
} Booking;


User    users[MAX_USERS];
Driver  drivers[MAX_DRIVERS];
Booking bookings[MAX_BOOKINGS];
int     userCount    = 0;
int     driverCount  = 0;
int     bookingCount = 0;
int     loggedInUser = -1;   /* index into users[]  */
int     loggedInDriver = -1; /* index into drivers[] */
int     isAdmin      = 0;
