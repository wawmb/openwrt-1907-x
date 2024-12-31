// SPDX-License-Identifier: GPL-3.0+
/* Copyright (c) 2021 Motor-comm Corporation.
 * Confidential and Proprietary. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "IniInterface.h"

#define PRINT_ERRMSG(STR) fprintf(stderr, "line:%d,msg:%s,eMsg:%s\n", __LINE__, STR, strerror(errno))

#define TRIM_LIFT 1  
#define TRIM_RIGHT 2 
#define TRIM_SPACE 3 


bool str_empty(const char* string)
{
    return NULL == string || '\0' == *string;
}

bool cnf_add_option(Config* cnf, const char* section, const char* key, const char* value)
{
    if (NULL == cnf || str_empty(section) || str_empty(key) || str_empty(value))
    {
        return false; 
    }

    Data* pData = cnf->data; 
    while (NULL != pData && 0 != strcmp(pData->section, section))
    {
        pData = pData->next;
    }

    if (NULL == pData)
    { 
        Data* ps = (Data*)malloc(sizeof(Data));
        if (NULL == ps)
        {
            return false;
        }
        
        ps->section = (char*)malloc(sizeof(char) * (strlen(section) + 1));
        strcpy(ps->section, section);
        ps->option = NULL;    
        ps->next = cnf->data; 
        cnf->data = pData = ps;
    }

    Option* pOption = pData->option;
    Option* pFather = pOption;
    while (NULL != pOption && 0 != strcmp(pOption->key, key))
    {
        pFather = pOption;
        pOption = pOption->next;
    }
    
    if (NULL == pOption)
    { 
        pOption = (Option*)malloc(sizeof(Option));
        if (NULL == pOption)
            return false;
        
        pOption->key = (char*)malloc(sizeof(char) * (strlen(key) + 1));
        strcpy(pOption->key, key);
        //pOption->next = pData->option;
        //pData->option = pOption;
        if (pFather)
            pFather->next = pOption;
        else
            pData->option = pOption;
        
        pOption->value = (char*)malloc(sizeof(char) * (strlen(value) + 1));

        pOption->next = NULL;
    }
    else if (strlen(pOption->value) < strlen(value))
    {
        pOption->value = (char*)realloc(pOption->value, sizeof(char) * (strlen(value) + 1));
    }

    strcpy(pOption->value, value);

    return true;
}

char* trim_string(char* string, int mode)
{
    char* left = string;
    if ((mode & TRIM_LIFT) != 0)
    {         for (; *left != '\0'; left++)
        {
            if (0 == isspace(*left))
            {
                break;
            }
        }
    }
    if ((mode & TRIM_RIGHT) != 0)
    {         char* right = string - 1 + strlen(string);
        for (; right >= left; right--)
        {
            if (0 == isspace(*right))
            {
                *(right + 1) = '\0';
                break;
            }
        }
    }
    return left;
}

Config* cnf_read_config(const char* filename, char comment, char separator)
{
    Config* cnf = (Config*)malloc(sizeof(Config));
    if (NULL == cnf)
    {
        return NULL;
    }
    cnf->comment = comment;     
    cnf->separator = separator; 
    cnf->data = NULL;           

    if (str_empty(filename))
    {
        return cnf; 
    }

    FILE* fp = fopen(filename, "r");
    if (NULL == fp)
    {
        PRINT_ERRMSG("fopen");
        return NULL;
    }

#define MAX_VALUE 1024 

    char* s, * e, * pLine, sLine[MAX_VALUE];                              
    char section[MAX_VALUE] = { '\0' }, key[MAX_VALUE], value[MAX_VALUE]; 
    while (NULL != fgets(sLine, MAX_VALUE, fp))
    {
        pLine = trim_string(sLine, TRIM_SPACE); 
        if (*pLine == '\0' || *pLine == comment)
        {
            continue; 
        }
        s = strchr(pLine, comment);
        if (s != NULL)
        {
            *s = '\0'; 
        }

        s = strchr(pLine, '[');
        if (s != NULL)
        {
            e = strchr(++s, ']');
            if (e != NULL)
            {
                *e = '\0'; 
                strcpy(section, s);
            }
        }
        else
        {
            s = strchr(pLine, separator);
            if (s != NULL && *section != '\0')
            {                                               
                *s = '\0';                                    
                strcpy(key, trim_string(pLine, TRIM_RIGHT));  
                strcpy(value, trim_string(s + 1, TRIM_LIFT)); 
                cnf_add_option(cnf, section, key, value);     
            }
        }
    } 
    fclose(fp);
    return cnf;
}

bool cnf_get_value(Config* cnf, const char* section, const char* key)
{
    Data* p = cnf->data; 
    while (NULL != p && 0 != strcmp(p->section, section))
    {
        p = p->next;
    }

    if (NULL == p)
    {
        PRINT_ERRMSG("section not find!");
        return false;
    }

    Option* q = p->option;
    while (NULL != q && 0 != strcmp(q->key, key))
    {
        q = q->next; 
    }

    if (NULL == q)
    {
        PRINT_ERRMSG("key not find!");
        return false;
    }

    cnf->re_string = q->value;                          
    cnf->re_int = atoi(cnf->re_string);                 
    cnf->re_bool = 0 == strcmp("true", cnf->re_string); 
    cnf->re_double = atof(cnf->re_string);              
    return true;
}

Data* cnf_has_section(Config* cnf, const char* section)
{
    Data* p = cnf->data; 
    while (NULL != p && 0 != strcmp(p->section, section))
    {
        p = p->next;
    }

    if (NULL == p)
    { 
        return NULL;
    }

    return p;
}

Option* cnf_has_option(Config* cnf, const char* section, const char* key)
{
    Data* p = cnf_has_section(cnf, section);
    if (NULL == p)
    { 
        return NULL;
    }

    Option* q = p->option;
    while (NULL != q && 0 != strcmp(q->key, key))
    {
        q = q->next; 
    }
    if (NULL == q)
    { 
        return NULL;
    }

    return q;
}

bool cnf_write_file(Config* cnf, const char* filename, const char* header)
{
    FILE* fp = fopen(filename, "w");
    if (NULL == fp)
    {
        PRINT_ERRMSG("fopen");
        return false;
    }

    if (!str_empty(header))
    { 
        fprintf(fp, "%c %s\n\n", cnf->comment, header);
    }

    Option* q;
    Data* p = cnf->data;
    while (NULL != p)
    {
        fprintf(fp, "[%s]\n", p->section);
        q = p->option;
        while (NULL != q)
        {
            fprintf(fp, "%s %c %s\n", q->key, cnf->separator, q->value);
            q = q->next;
        }
        p = p->next;
    }

    fclose(fp);
    return true;
}

bool cnf_remove_option(Config* cnf, const char* section, const char* key)
{
    Data* ps = cnf_has_section(cnf, section);
    if (NULL == ps)
    { 
        return false;
    }

    Option* p, * q;
    q = p = ps->option;
    while (NULL != p && 0 != strcmp(p->key, key))
    {
        if (p != q)
        {
            q = q->next;
        } 
        p = p->next;
    }

    if (NULL == p)
    { 
        return false;
    }

    if (p == q)
    { 
        ps->option = p->next;
    }
    else
    {
        q->next = p->next;
    }

    free(p->key);
    free(p->value);
    free(p);
    q = p = NULL; 
    return true;
}

bool cnf_remove_section(Config* cnf, const char* section)
{
    if (str_empty(section))
    {
        return false;
    }

    Data* p, * q;
    q = p = cnf->data; 
    while (NULL != p && 0 != strcmp(p->section, section))
    {
        if (p != q)
        {
            q = q->next;
        } 
        p = p->next;
    }

    if (NULL == p)
    { 
        return false;
    }

    if (p == q)
    { 
        cnf->data = p->next;
    }
    else
    { 
        q->next = p->next;
    }

    Option* ot, * o = p->option;
    while (NULL != o)
    { 
        free(o->key);
        free(o->value);
        ot = o;
        o = o->next;
        free(ot);
    }
    free(p);      
    q = p = NULL; 
    return true;
}

void destroy_config(Config** cnf)
{
    if (NULL == *cnf)
        return;

    if (NULL != (*cnf)->data)
    {
        Data* pt, * p = (*cnf)->data;
        Option* qt, * q;
        while (NULL != p)
        {
            q = p->option;
            while (NULL != q)
            {
                free(q->key);
                free(q->value); 

                qt = q;
                q = q->next;
                free(qt);
            }
            free(p->section); 

            pt = p;
            p = p->next;
            free(pt);
        }
    }
    free(*cnf);
    *cnf = NULL;
}

void print_config(Config* cnf)
{
    Data* p = cnf->data;     while (NULL != p)
    {
        printf("[%s]\n", p->section);

        Option* q = p->option;
        while (NULL != q)
        {
            printf("%s%c%s\n", q->key, cnf->separator, q->value);
            q = q->next;
        }
        p = p->next;
    }
}