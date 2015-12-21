/**
 * @file
 * Log API
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _LOG_H_
#define _LOG_H_


#ifdef __cplusplus
extern "C" {
#endif

void Loge (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));
void Logi (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));


#ifdef __cplusplus
}
#endif


#endif /* _LOG_H_ */
