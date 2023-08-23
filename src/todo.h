/**
 * Tracking of TODOs
 **/

#pragma once

/**
 * @brief Show a message box indicating that this function is not implemented yet
 * @param functionName - source function name
 **/
void ShowNotImplementedMessage(const char *functionName);

/**
 * Macro to call ShowNotImplementedMessage() with the current function name
 **/
#define TODO_NOT_IMPLEMENTED ShowNotImplementedMessage(__FUNCTION__);