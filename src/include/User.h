/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef TELODENDRIA_USER_H
#define TELODENDRIA_USER_H

/***
 * @Nm User
 * @Nd Convenience functions for working with local users.
 * @Dd April 28 2023
 * @Xr Db
 *
 * The
 * .Nm
 * API provides a wrapper over the database and offers an easy way to
 * manage local users. It supports all of the locking mechanisms that
 * the database does, and provides features for authenticating local
 * users, among many other tasks.
 */

#include <Cytoplasm/Int64.h>
#include <Cytoplasm/Db.h>
#include <Cytoplasm/Json.h>

/**
 * Many functions here operate on an opaque user structure.
 */
typedef struct User User;

/**
 * Local users can have privileges to access the administrator API.
 * These are the individual privileges that Telodendria supports.
 * Note that they are bit flags, so they can be bitwise OR-ed together
 * to have multiple privileges.
 */
typedef enum UserPrivileges
{
    USER_NONE = 0,
    USER_DEACTIVATE = (1 << 0),
    USER_ISSUE_TOKENS = (1 << 1),
    USER_CONFIG = (1 << 2),
    USER_GRANT_PRIVILEGES = (1 << 3),
    USER_PROC_CONTROL = (1 << 4),
    USER_ALIAS = (1 << 5),
    USER_ALL = ((1 << 6) - 1)
} UserPrivileges;

/**
 * A description of an access token, which users use to authenticate
 * with the client-server API.
 */
typedef struct UserAccessToken
{
    char *user;
    char *string;
    char *deviceId;
    Int64 lifetime;
} UserAccessToken;

/**
 * Login information, which is in most cases returned to the user
 * upon a successful login.
 */
typedef struct UserLoginInfo
{
    UserAccessToken *accessToken;
    char *refreshToken;
} UserLoginInfo;

/**
 * A description of a Matrix user ID.
 */
typedef struct UserId
{
    char *localpart;
    char *server;
} UserId;

/**
 * Take a localpart and domain as separate parameters and validate them
 * against the rules of the Matrix specification. The reasion the
 * domain is required is because the spec imposes limitations on the
 * length of the user ID, so the longer the domain name is, the shorter
 * the local part is allowed to be. This function is used to ensure
 * that client-provided Matrix IDs are valid on this server.
 */
extern int UserValidate(char *, char *);

/**
 * This function behaves just like
 * .Fn UserValidate ,
 * except that it is a little more lenient in what is considers to be
 * a valid Matrix ID. This is typically to validate users that exist
 * on other servers, since some usernames may exist that are not fully
 * spec compliant but remain in use since before the new restrictions
 * were put in place.
 */
extern int UserHistoricalValidate(char *, char *);

/**
 * Determine whether the user identified by the specified localpart
 * exists in the database.
 */
extern int UserExists(Db *, char *);

/**
 * Create a new user with the specified localpart and password, in
 * that order.
 */
extern User * UserCreate(Db *, char *, char *);

/**
 * Take a localpart and obtain a database reference to the user
 * identified by that localpart. This function behaves analogously
 * to
 * .Fn DbLock ,
 * and in fact it uses it under the hood to ensure that the user can
 * only be modified by the thread that has locked it.
 */
extern User * UserLock(Db *, char *);

/**
 * Take an access token, figure out what user it belongs to, and then
 * returns a reference to that user. This function should be used by
 * most endpoints that require user authentication, since most
 * endpoints are authenticated via access tokens.
 */
extern User * UserAuthenticate(Db *, char *);

/**
 * Return a user reference back to the database. This function uses
 * .Fn DbUnlock
 * under the hood.
 */
extern int UserUnlock(User *);

/**
 * Log in a user. This function takes the user's password, desired
 * device ID and display name, and a boolean value indicating whether 
 * or not the client supports refresh tokens. This function logs the
 * the user in and generates an access token to be returned to the
 * client.
 */
extern UserLoginInfo * UserLogin(User *, char *, char *, char *, int);

/**
 * Get the localpart attached to a user object. This function may be
 * useful in the few cases where the localpart is not known already.
 */
extern char * UserGetName(User *);

/**
 * Get the device ID attached to a user object, or NULL if the user
 * reference was not obtained using
 * .Fn UserAuthenticate .
 * If
 * .Fn UserLogin
 * is used, the return value will have the device ID in it, but the
 * device ID is not set on the user reference.
 */
extern char * UserGetDeviceId(User *);

/**
 * Take a password and verify it against a user object. Telodendria
 * does not store passwords in plain text, so this function hashes the
 * password and checks it against what is stored in the database.
 */
extern int UserCheckPassword(User *, char *);

/**
 * Reset the given user's password by hashing a plain text password and
 * storing it in the database.
 */
extern int UserSetPassword(User *, char *);

/**
 * Immediately deactivate the given user account such that it can no
 * longer be used to log in, but the username is still reserved. This
 * is to prevent future users from pretending to be a previous user
 * of a given localpart. The user is logged out; all access tokens are
 * invalidated.
 * Additionally, it stores information on who deactivated the account 
 * (be it an admin or the user itself), and why. If the user
 * responsible for deactivating the target user is NULL, then it is 
 * set to the target's own name.
 */
extern int UserDeactivate(User *, char *, char *);

/**
 * Reactivates the given user account if it has been deactvated with 
 * .Fn UserDeactivate ,
 * otherwise, it simply doesn't do anything.
 */
extern int UserReactivate(User *);

/**
 * Return a boolean value indicating whether or not the user was
 * deactivated using
 * .Fn UserDeactivate .
 */
extern int UserDeactivated(User *);

/**
 * Fetches the devices that belong to the user, in JSON format,
 * identical to what's stored in the database. In fact, this JSON is
 * still linked to the database, so it should not be freed with
 * .Fn JsonFree .
 */
extern HashMap * UserGetDevices(User *);

/**
 * Generate a new access token for the given user. This is mainly
 * used internally. It takes the device ID and a boolean value
 * indicating whether or not the token should expire.
 */
extern UserAccessToken * UserAccessTokenGenerate(User *, char *, int);

/**
 * Write the specified access token to the database, returning a
 * boolean value indicating success.
 */
extern int UserAccessTokenSave(Db *, UserAccessToken *);

/**
 * Free the memory associated with the given access token.
 */
extern void UserAccessTokenFree(UserAccessToken *);

/**
 * Delete a specific access token by name.
 */
extern int UserDeleteToken(User *, char *);

/**
 * Get a string property from the user's profile given the specified
 * key.
 */
extern char * UserGetProfile(User *, char *);

/**
 * Set a string property on the user's profile. A key/value pair should
 * be provided.
 */
extern void UserSetProfile(User *, char *, char *);

/**
 * Delete all of the access tokens that belong to the specified user,
 * except for the one provided by name, unless NULL is provided for
 * the name.
 */
extern int UserDeleteTokens(User *, char *);

/**
 * Get the current privileges of the user as a packed bit field. Use
 * the flags defined in UserPrivileges to deterine what privileges a
 * user has.
 */
extern int UserGetPrivileges(User *);

/**
 * Set the privileges of the user.
 */
extern int UserSetPrivileges(User *, int);

/**
 * Decode the JSON that represents the user privileges into a packed
 * bit field for simple manipulation.
 */
extern int UserDecodePrivileges(JsonValue *);

/**
 * Encode the packed bit field that represents user privileges as a
 * JSON value.
 */
extern JsonValue *UserEncodePrivileges(int);

/**
 * Convert a string privilege into its bit in the bit field. This is
 * mainly intended to be used internally. At the time of writing, I
 * don't recall exactly why it's in the public API.
 */
extern int UserDecodePrivilege(const char *);

/**
 * Parse either a localpart or a fully qualified Matrix ID. If the
 * first argument is a localpart, then the second argument is used as
 * the server name.
 */
extern UserId * UserIdParse(char *, char *);

/**
 * Free the memory associated with the parsed Matrix ID.
 */
extern void UserIdFree(UserId *);

#endif                             /* TELODENDRIA_USER_H */
