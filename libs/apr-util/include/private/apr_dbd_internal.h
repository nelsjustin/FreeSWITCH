/* Copyright 2000-2005 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Overview of what this is and does:
 * http://www.apache.org/~niq/dbd.html
 */

#ifndef APR_DBD_INTERNAL_H
#define APR_DBD_INTERNAL_H

#include <stdarg.h>

#include "apr_dbd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct apr_dbd_driver_t {
    /** name */
    const char *name;

    /** init: allow driver to perform once-only initialisation.
     *  Called once only.  May be NULL
     */
    void (*init)(apr_pool_t *pool);

    /** native_handle: return the native database handle of the underlying db
     *
     * @param handle - apr_dbd handle
     * @return - native handle
     */
    void *(*native_handle)(apr_dbd_t *handle);

    /** open: obtain a database connection from the server rec.
     *  Must be explicitly closed when you're finished with it.
     *  WARNING: only use this when you need a connection with
     *  a lifetime other than a request
     *
     *  @param pool - a pool to use for error messages (if any).
     *  @param s - server rec managing the underlying connection/pool.
     *  @return database handle, or NULL on error.
     */
    apr_dbd_t *(*open)(apr_pool_t *pool, const char *params);

    /** check_conn: check status of a database connection
     *
     *  @param pool - a pool to use for error messages (if any).
     *  @param handle - the connection to check
     *  @return APR_SUCCESS or error
     */
    apr_status_t (*check_conn)(apr_pool_t *pool, apr_dbd_t *handle);

    /** close: close/release a connection obtained from open()
     *
     *  @param handle - the connection to release
     *  @return APR_SUCCESS or error
     */
    apr_status_t (*close)(apr_dbd_t *handle);

    /** set_dbname: select database name.  May be a no-op if not supported.
     *
     *  @param pool - working pool
     *  @param handle - the connection
     *  @param name - the database to select
     *  @return 0 for success or error code
     */
    int (*set_dbname)(apr_pool_t* pool, apr_dbd_t *handle, const char *name);

    /** transaction: start a transaction.  May be a no-op.
     *
     *  @param pool - a pool to use for error messages (if any).
     *  @param handle - the connection
     *  @param transaction - ptr to a transaction.  May be null on entry
     *  @return 0 for success or error code
     */
    int (*start_transaction)(apr_pool_t *pool, apr_dbd_t *handle,
                             apr_dbd_transaction_t **trans);

    /** end_transaction: end a transaction
     *  (commit on success, rollback on error).
     *  May be a no-op.
     *
     *  @param transaction - the transaction.
     *  @return 0 for success or error code
     */
    int (*end_transaction)(apr_dbd_transaction_t *trans);

    /** query: execute an SQL query that doesn't return a result set
     *
     *  @param handle - the connection
     *  @param nrows - number of rows affected.
     *  @param statement - the SQL statement to execute
     *  @return 0 for success or error code
     */
    int (*query)(apr_dbd_t *handle, int *nrows, const char *statement);

    /** select: execute an SQL query that returns a result set
     *
     *  @param pool - pool to allocate the result set
     *  @param handle - the connection
     *  @param res - pointer to result set pointer.  May point to NULL on entry
     *  @param statement - the SQL statement to execute
     *  @param random - 1 to support random access to results (seek any row);
     *                  0 to support only looping through results in order
     *                    (async access - faster)
     *  @return 0 for success or error code
     */
    int (*select)(apr_pool_t *pool, apr_dbd_t *handle, apr_dbd_results_t **res,
                  const char *statement, int random);

    /** num_cols: get the number of columns in a results set
     *
     *  @param res - result set.
     *  @return number of columns
     */
    int (*num_cols)(apr_dbd_results_t *res);

    /** num_tuples: get the number of rows in a results set
     *  of a synchronous select
     *
     *  @param res - result set.
     *  @return number of rows, or -1 if the results are asynchronous
     */
    int (*num_tuples)(apr_dbd_results_t *res);

    /** get_row: get a row from a result set
     *
     *  @param pool - pool to allocate the row
     *  @param res - result set pointer
     *  @param row - pointer to row pointer.  May point to NULL on entry
     *  @param rownum - row number, or -1 for "next row".  Ignored if random
     *                  access is not supported.
     *  @return 0 for success, -1 for rownum out of range or data finished
     */
    int (*get_row)(apr_pool_t *pool, apr_dbd_results_t *res,
                   apr_dbd_row_t **row, int rownum);
  
    /** get_entry: get an entry from a row
     *
     *  @param row - row pointer
     *  @param col - entry number
     *  @param val - entry to fill
     *  @return 0 for success, -1 for no data, +1 for general error
     */
    const char* (*get_entry)(const apr_dbd_row_t *row, int col);
  
    /** error: get current error message (if any)
     *
     *  @param handle - the connection
     *  @param errnum - error code from operation that returned an error
     *  @return the database current error message, or message for errnum
     *          (implementation-dependent whether errnum is ignored)
     */
    const char *(*error)(apr_dbd_t *handle, int errnum);
  
    /** escape: escape a string so it is safe for use in query/select
     *
     *  @param pool - pool to alloc the result from
     *  @param string - the string to escape
     *  @param handle - the connection
     *  @return the escaped, safe string
     */
    const char *(*escape)(apr_pool_t *pool, const char *string,
                          apr_dbd_t *handle);
  
    /** prepare: prepare a statement
     *
     *  @param pool - pool to alloc the result from
     *  @param handle - the connection
     *  @param query - the SQL query
     *  @param label - A label for the prepared statement.
     *                 use NULL for temporary prepared statements
     *                 (eg within a Request in httpd)
     *  @param statement - statement to prepare.  May point to null on entry.
     *  @return 0 for success or error code
     */
    int (*prepare)(apr_pool_t *pool, apr_dbd_t *handle, const char *query,
                   const char *label, apr_dbd_prepared_t **statement);

    /** pvquery: query using a prepared statement + args
     *
     *  @param pool - working pool
     *  @param handle - the connection
     *  @param nrows - number of rows affected.
     *  @param statement - the prepared statement to execute
     *  @param args - args to prepared statement
     *  @return 0 for success or error code
     */
    int (*pvquery)(apr_pool_t *pool, apr_dbd_t *handle, int *nrows,
                   apr_dbd_prepared_t *statement, va_list args);

    /** pvselect: select using a prepared statement + args
     *
     *  @param pool - working pool
     *  @param handle - the connection
     *  @param res - pointer to query results.  May point to NULL on entry
     *  @param statement - the prepared statement to execute
     *  @param random - Whether to support random-access to results
     *  @param args - args to prepared statement
     *  @return 0 for success or error code
     */
    int (*pvselect)(apr_pool_t *pool, apr_dbd_t *handle,
                    apr_dbd_results_t **res,
                    apr_dbd_prepared_t *statement, int random, va_list args);

    /** pquery: query using a prepared statement + args
     *
     *  @param pool - working pool
     *  @param handle - the connection
     *  @param nrows - number of rows affected.
     *  @param statement - the prepared statement to execute
     *  @param nargs - number of args to prepared statement
     *  @param args - args to prepared statement
     *  @return 0 for success or error code
     */
    int (*pquery)(apr_pool_t *pool, apr_dbd_t *handle, int *nrows,
                  apr_dbd_prepared_t *statement, int nargs,
                  const char **args);

    /** pselect: select using a prepared statement + args
     *
     *  @param pool - working pool
     *  @param handle - the connection
     *  @param res - pointer to query results.  May point to NULL on entry
     *  @param statement - the prepared statement to execute
     *  @param random - Whether to support random-access to results
     *  @param nargs - number of args to prepared statement
     *  @param args - args to prepared statement
     *  @return 0 for success or error code
     */
    int (*pselect)(apr_pool_t *pool, apr_dbd_t *handle,
                   apr_dbd_results_t **res, apr_dbd_prepared_t *statement,
                   int random, int nargs, const char **args);


};


#ifdef __cplusplus
}
#endif

#endif
