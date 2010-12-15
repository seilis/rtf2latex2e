// MathType equation to LaTeX converter
//
// Ported 2000.04.03 by Steve Swanson, steve@mackichan.com
// Initial implementation by Jack Medd.  Originally part of
// RTF to LaTeX converter in Scientific WorkPlace (http://www.mackichan.com).
//
// The MathType equation format is described at
//   http://www.mathtype.com/support/tech/MTEF4.htm
//   http://www.mathtype.com/support/tech/MTEF5.htm
//   http://www.mathtype.com/support/tech/MTEF_storage.htm
//   http://www.mathtype.com/support/tech/encodings/mtcode.stm
// Various undocumented details determined by debugging and intuition.
//

# include	<stdio.h>
# include	<string.h>
# include	<stdlib.h>
# include	<ctype.h>

# include	"rtf.h"
# include	"rtf2LaTeX2e.h"

# include "eqn.h"

typedef struct {
  int       log_level;
  int       do_delete;
  int       ilk;
  int       is_line;
  char*     data;
} EQ_STRREC;

static MT_OBJLIST* Eqn_GetObjectList(MTEquation* eqn, unsigned char* eqn_stream, int* index, int count);
static MT_LINE*    Eqn_inputLINE(    MTEquation* eqn, unsigned char* src,int* delta );
static MT_CHAR*    Eqn_inputCHAR(    MTEquation* eqn, unsigned char* src,int* delta );
static MT_TMPL*    Eqn_inputTMPL(    MTEquation* eqn, unsigned char* src,int* delta );
static MT_PILE*    Eqn_inputPILE(    MTEquation* eqn, unsigned char* src,int* delta );
static MT_MATRIX*  Eqn_inputMATRIX(  MTEquation* eqn, unsigned char* src,int* delta );
static MT_EMBELL*  Eqn_inputEMBELL(  MTEquation* eqn, unsigned char* src,int* delta );
static MT_RULER*   Eqn_inputRULER(   MTEquation* eqn, unsigned char* src,int* delta );
static MT_FONT*    Eqn_inputFONT(    MTEquation* eqn, unsigned char* src,int* delta );
static MT_SIZE*    Eqn_inputSIZE(    MTEquation* eqn, unsigned char* src,int* delta );

static int  GetNudge(unsigned char* src,int* x,int* y );

static void DeleteObjectList( MT_OBJLIST* the_list );
static void DeleteTabstops(   MT_TABSTOP* the_list );
static void DeleteEmbells(    MT_EMBELL*  the_list );

static char* Eqn_TranslateObjects(MTEquation* eqn,MT_OBJLIST* the_list );
static char* Eqn_TranslateLINE(   MTEquation* eqn,MT_LINE* line );
static char* Eqn_TranslateFUNCTION(      MTEquation* eqn,MT_OBJLIST* curr_node,int* advance );
static char* Eqn_TranslateTEXTRUN(       MTEquation* eqn,MT_OBJLIST* curr_node,int* advance );
static char* Eqn_TranslateCHAR(          MTEquation* eqn,MT_CHAR* thechar );
static char* Eqn_TranslateTMPL(          MTEquation* eqn,MT_TMPL* tmpl );
static char* Eqn_TranslateLINE(          MTEquation* eqn,MT_LINE* line );
static char* Eqn_TranslatePILE(          MTEquation* eqn,MT_PILE* pile );
static char* Eqn_TranslateMATRIX(        MTEquation* eqn,MT_MATRIX* matrix );
static char* Eqn_TranslateFONT(          MTEquation* eqn,MT_FONT* font );
static char* Eqn_TranslateRULER(         MTEquation* eqn,MT_RULER* ruler );
static char* Eqn_TranslateSIZE(          MTEquation* eqn,MT_SIZE* size );
static char* Eqn_TranslateEQNARRAY(      MTEquation* eqn,MT_PILE* pile );

static int Eqn_GetTmplStr(      MTEquation* eqn,int selector,int variation,EQ_STRREC* strs );
static int Eqn_GetTexChar(      MTEquation* eqn,EQ_STRREC* strs,MT_CHAR* thechar,int* math_attr );
static void Eqn_LoadCharSetAtts(MTEquation* eqn,char** table);
static void GetPileType( char* the_template,int arg_num,char* targ_nom );

static int GetProfileStr(char** section, char* key, char* data, int datalen);

static void BreakTeX( char* ztex,FILE* outfile );
static char* ToBuffer( char* src,char* buffer,int* off,int* lim );
static void SetComment( EQ_STRREC* strs,int lev,char* src );
static void SetDollar( EQ_STRREC* strs,int turn_on );
static char* Eqn_JoinzStrs( MTEquation* eqn,EQ_STRREC* strs,int num_strs );


#define NUM_TYPEFACE_SLOTS    32

#define Z_TEX         1
#define Z_COMMENT     2
#define Z_TMPL        3

#define MA_NONE         0
#define MA_FORCE_MATH   1
#define MA_FORCE_TEXT   2


// Various data which used to be in rtf2latex.ini.  Initialized at bottom of file.
char* Profile_FUNCTIONS[];
char* Profile_VARIABLES[];
char* Profile_PILEtranslation[];
char* Profile_CHARSETatts[];
char* Profile_CHARSETatts3[];
char* Profile_CHARTABLE3[];
char* Profile_CHARTABLE[];
char* Profile_TEMPLATES[];

char* Template_EMBELLS[];


/* MathType Equation converter */

boolean Eqn_Create(MTEquation* eqn, unsigned char *eqn_stream, int eqn_size)
{
int src_index;

  eqn->indent[0]  =  '%';
  eqn->indent[1]  =  0;
  eqn->o_list     = NULL;
  eqn->atts_table = NULL;

  eqn->text_mode =  0;

  eqn->m_mtef_ver    = eqn_stream[0];
  if (eqn->m_mtef_ver == 1 || eqn->m_mtef_ver == 101) {  // MathType version 1
    eqn->m_platform    = 0;
    eqn->m_product     = 0;
    eqn->m_version     = 0;
    eqn->m_version_sub = 0;
    src_index = 1;
  } else if (eqn->m_version == 4 || (eqn->m_version == 3 && eqn->m_version_sub >= 5)) {
    printf ("* Unsupported MathType Equation version %d.%d\n", eqn->m_version, eqn->m_version_sub); 
    return(false);
  } else {
    eqn->m_platform    = eqn_stream[1];
    eqn->m_product     = eqn_stream[2];
    eqn->m_version     = eqn_stream[3];
    eqn->m_version_sub = eqn_stream[4];
    src_index =  5;
  }
  if (eqn->m_mtef_ver == 3) {
    eqn->m_atts_table = Profile_CHARSETatts3;
    Eqn_LoadCharSetAtts(eqn,Profile_CHARSETatts3);
    eqn->m_char_table = Profile_CHARTABLE3;
  } else {
    eqn->m_atts_table = Profile_CHARSETatts;
    Eqn_LoadCharSetAtts(eqn,Profile_CHARSETatts);
    eqn->m_char_table = Profile_CHARTABLE;
  }

  // We expect a SIZE then a LINE or PILE
  eqn->o_list = Eqn_GetObjectList( eqn,eqn_stream,&src_index,2 );

  return(true);
}

void Eqn_Destroy(MTEquation* eqn)
{
  if (eqn->o_list) {
    DeleteObjectList(eqn->o_list);
    eqn->o_list = NULL;
  }
  if (eqn->atts_table) {
    free( eqn->atts_table );
    eqn->atts_table = NULL;
  }
}

void Eqn_TranslateObjectList(MTEquation* eqn, FILE* outfile, int loglevel)
{
  char* ztex;

  eqn->out_file  =  outfile;
  eqn->log_level =  loglevel;

  if ( eqn->log_level == 2 )
    fputs( "%Begin Equation\n",eqn->out_file );
  else
    fputs( "\n",eqn->out_file );

  eqn->math_mode =  0;

  ztex  =  Eqn_TranslateObjects( eqn,eqn->o_list );
  if ( ztex ) {
    BreakTeX( ztex,eqn->out_file );
    free(ztex);
  }

  if ( eqn->math_mode )
    fputs( "$",eqn->out_file );

  if ( eqn->log_level == 2 )
    fputs( " %End Equation\n",eqn->out_file );
  else
    fputs( "\n",eqn->out_file );
}

#define zLINE_MAX   75

static
void BreakTeX( char* ztex,FILE* outfile ) {

  char* start =  ztex;
  int char_count  =  0;
  int last_space  =  0;
  int put_line    =  0;
  char ch;
  while ( (ch=*ztex) ) {
    if        ( ch=='\\' ) {
      char_count++;
      ztex++;
      char_count++;
      ztex++;

    } else if ( ch=='%' ) {

      while ( *ztex && *ztex!='\r' && *ztex!='\n' ) ztex++;
      if ( *ztex ) *ztex++ =  0;
      put_line  =  1;

    } else if ( ch==' ' ) {
      last_space  =  char_count++;
      ztex++;

    } else if ( ch=='\r' || ch=='\n' ) {
      *ztex++ =  0;
      put_line  =  1;

    } else {
      char_count++;
      ztex++;
    }

    if ( put_line==0 && char_count>=zLINE_MAX ) {
      if ( last_space ) {
        ztex  =  start+last_space;
        *ztex++ =  0;
        put_line  =  1;
      }
    }
    if ( put_line ) {
      fputs( start,outfile );
      fputc( '\n',outfile );
      while ( *ztex && *ztex<=' ' ) ztex++;
      start =  ztex;
      char_count  =  0;
      last_space  =  0;
      put_line  =  0;
    }
  }

  if ( char_count )
    fputs( start,outfile );
}


/////////////////////////////////////////////////////////////////////////////
// static, local, Private...whatever
/////////////////////////////////////////////////////////////////////////////


// scanning routines.  Convert MT equation into internal form
MT_OBJLIST* Eqn_GetObjectList(MTEquation* eqn, unsigned char* src, int* src_index, int num_objs)
{

  int tally =  0;
  MT_OBJLIST* head  =  (MT_OBJLIST*)NULL;
  MT_OBJLIST* curr;
  void* new_obj;

  unsigned char curr_tag =  *(src+*src_index) & 0x0F;
  unsigned char attrs;

  while ( curr_tag!=END && curr_tag!= 0xFF ) {

    new_obj =  (void*)NULL;

    switch ( curr_tag ) {

      case LINE   : {
        attrs  =  *(src+*src_index);
        if ( attrs & xfNULL )
          (*src_index)++;
        else
          new_obj =  (void*)Eqn_inputLINE( eqn,src,src_index );
      }
      break;
      case CHAR   : new_obj =  (void*)Eqn_inputCHAR( eqn,src,src_index );     break;
      case TMPL   : new_obj =  (void*)Eqn_inputTMPL( eqn,src,src_index );     break;
      case PILE   : new_obj =  (void*)Eqn_inputPILE( eqn,src,src_index );     break;
      case MATRIX : new_obj =  (void*)Eqn_inputMATRIX( eqn,src,src_index );   break;

      case EMBELL : break;

      case RULER  : new_obj =  (void*)Eqn_inputRULER( eqn,src,src_index );    break;
      case FONT   : new_obj =  (void*)Eqn_inputFONT( eqn,src,src_index );     break;
      case SIZE   :
      case FULL   :
      case SUB    :
      case SUB2   :
      case SYM    :
      case SUBSYM : new_obj =  (void*)Eqn_inputSIZE( eqn,src,src_index );     break;

      default :   break;
    }

    if ( new_obj ) {
      MT_OBJLIST* new_node  =  (MT_OBJLIST*)malloc(sizeof(MT_OBJLIST));
      new_node->next    =  NULL;
      new_node->tag     =  curr_tag;
      new_node->obj_ptr =  new_obj;

      if ( head ) curr->next  =  (void *) new_node;
      else        head  =  new_node;
      curr  =  new_node;
    }

    curr_tag  =  *(src+*src_index) & 0x0F;

    tally++;
    if ( tally==num_objs ) return head;

  }     // while loop thru MathType Objects

  (*src_index)++;            // step over end byte

  return head;
}

MT_LINE* Eqn_inputLINE( MTEquation* eqn,unsigned char* src,int* src_index ) {
  unsigned char attrs  =  *(src+*src_index);

  MT_LINE* new_line =  (MT_LINE*)malloc(sizeof(MT_LINE));

  (*src_index)++;                    // step over tag
  if ( attrs & xfLMOVE )
    *src_index +=  GetNudge( src+*src_index,&new_line->nudge_x,&new_line->nudge_y );
  else {
    new_line->nudge_x =  0;
    new_line->nudge_y =  0;
  }

  if ( attrs & xfLSPACE ) {
    new_line->line_spacing  =  *(src+*src_index);
    (*src_index)++;
  } else
    new_line->line_spacing  =  0;

  if ( attrs & xfRULER )
    new_line->ruler =  Eqn_inputRULER( eqn,src,src_index );
  else
    new_line->ruler =  (MT_RULER*)NULL;

  if ( attrs & xfNULL )
    new_line->object_list =  (MT_OBJLIST*)NULL;
  else
    new_line->object_list =  Eqn_GetObjectList( eqn,src,src_index,0 );

  return new_line;
}


MT_CHAR* Eqn_inputCHAR( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_CHAR* new_char =  (MT_CHAR*)malloc(sizeof(MT_CHAR));

  unsigned char attrs  =  *(src+*src_index);
  (*src_index)++;
  if ( attrs & xfLMOVE )
    *src_index +=  GetNudge( src+*src_index,&new_char->nudge_x,&new_char->nudge_y );
  else {
    new_char->nudge_x =  0;
    new_char->nudge_y =  0;
  }

  new_char->atts      =  attrs&0xF0;
  new_char->typeface  =  *(src+*src_index);      (*src_index)++;
  new_char->character =  *(src+*src_index);      (*src_index)++;
  if (eqn->m_mtef_ver == 3) {
    new_char->character |= *(src+*src_index) << 8;  // high byte last
    (*src_index)++;
  }
  if ( attrs & xfEMBELL ) {
    new_char->embellishment_list  =  Eqn_inputEMBELL( eqn,src,src_index );
    if (eqn->m_mtef_ver == 3)
      (src_index)++;
  } else
    new_char->embellishment_list  =  (MT_EMBELL*)NULL;

  return new_char;
}


MT_TMPL* Eqn_inputTMPL( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_TMPL* new_tmpl =  (MT_TMPL*)malloc(sizeof(MT_TMPL));

  unsigned char attrs  =  *(src+*src_index);
  (*src_index)++;
  if ( attrs & xfLMOVE )
    *src_index +=  GetNudge( src+*src_index,&new_tmpl->nudge_x,&new_tmpl->nudge_y );
  else {
    new_tmpl->nudge_x =  0;
    new_tmpl->nudge_y =  0;
  }

  new_tmpl->selector  =  *(src+*src_index);      (*src_index)++;
  new_tmpl->variation =  *(src+*src_index);      (*src_index)++;
  new_tmpl->options   =  *(src+*src_index);      (*src_index)++;

  new_tmpl->subobject_list  =  Eqn_GetObjectList( eqn,src,src_index,0 );

  return new_tmpl;
}


MT_PILE* Eqn_inputPILE( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_PILE* new_pile =  (MT_PILE*)malloc(sizeof(MT_PILE));

  unsigned char attrs  =  *(src+*src_index);
  (*src_index)++;                    // Step over the tag
  if ( attrs & xfLMOVE )
    *src_index +=  GetNudge( src+*src_index,&new_pile->nudge_x,&new_pile->nudge_y );
  else {
    new_pile->nudge_x =  0;
    new_pile->nudge_y =  0;
  }
  new_pile->halign  =  *(src+*src_index);      (*src_index)++;
  new_pile->valign  =  *(src+*src_index);      (*src_index)++;

  if ( attrs & xfRULER )
    new_pile->ruler =  Eqn_inputRULER( eqn,src,src_index );
  else
    new_pile->ruler =  (MT_RULER*)NULL;

  new_pile->line_list =  Eqn_GetObjectList( eqn,src,src_index,0 );

  return new_pile;
}


MT_MATRIX* Eqn_inputMATRIX( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_MATRIX* new_matrix =  (MT_MATRIX*)malloc(sizeof(MT_MATRIX));
  int row_bytes;
  int col_bytes;
  int idx =  0;

  unsigned char attrs  =  *(src+*src_index);
  (*src_index)++;
  if ( attrs & xfLMOVE )
    *src_index +=  GetNudge( src+*src_index,&new_matrix->nudge_x,&new_matrix->nudge_y );
  else {
    new_matrix->nudge_x =  0;
    new_matrix->nudge_y =  0;
  }
  new_matrix->valign      =  *(src+*src_index);    (*src_index)++;
  new_matrix->h_just      =  *(src+*src_index);    (*src_index)++;
  new_matrix->v_just      =  *(src+*src_index);    (*src_index)++;
  new_matrix->rows        =  *(src+*src_index);    (*src_index)++;
  new_matrix->cols        =  *(src+*src_index);    (*src_index)++;

  row_bytes =  ( 2*(new_matrix->rows+1) + 7 ) / 8;
  while ( idx<row_bytes ) {
    if ( idx<MATR_MAX )
      new_matrix->row_parts[idx]  =  *(src+*src_index);
    (*src_index)++;
    idx++;
  }

  col_bytes =  ( 2*(new_matrix->cols+1) + 7 ) / 8;
  idx =  0;
  while ( idx<col_bytes ) {
    if ( idx<MATR_MAX )
      new_matrix->col_parts[idx]  =  *(src+*src_index);
    (*src_index)++;
    idx++;
  }

  new_matrix->element_list  =  Eqn_GetObjectList( eqn,src,src_index,0 );

  return new_matrix;
}


MT_EMBELL* Eqn_inputEMBELL( MTEquation* eqn,unsigned char* src,int* src_index ) {

  unsigned char attrs  =  *(src+*src_index);
  MT_EMBELL* head       = NULL;
  MT_EMBELL* new_embell = NULL;
  MT_EMBELL* curr       = NULL;

  do {

    (*src_index)++;     // step over the embell tag - "06" - one for every acc

    new_embell =  (MT_EMBELL*)malloc(sizeof(MT_EMBELL));
    new_embell->next  =  NULL;

    if ( attrs & xfLMOVE ) {
      *src_index +=  GetNudge( src+*src_index,&new_embell->nudge_x,&new_embell->nudge_y );
    } else {
      new_embell->nudge_x =  0;
      new_embell->nudge_y =  0;
    }
    new_embell->embell  =  *(src+*src_index);      
    (*src_index)++;

    if ( head ) 
      curr->next  =  (struct MT_EMBELL*) new_embell;
    else
      head  =  new_embell;
    curr  =  new_embell;

    attrs  =  *(src+*src_index);
  } while ( attrs );

  (*src_index)++;            // advance over end byte

  return head;
}


MT_RULER* Eqn_inputRULER( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_TABSTOP* head  =  NULL;
  MT_TABSTOP* curr;
  MT_TABSTOP* new_stop;
  int num_stops;
  int i =  0;
  short tmp;
  MT_RULER* new_ruler =  (MT_RULER*)malloc(sizeof(MT_RULER));

  (*src_index)++;              // step over ruler tag

  head  =  (MT_TABSTOP*)NULL;
  num_stops =  *(src+*src_index);          (*src_index)++;
  while ( i<num_stops ) {
    new_stop  =  (MT_TABSTOP*)malloc(sizeof(MT_TABSTOP));
    new_stop->next    =  NULL;
    new_stop->type    =  *(src+*src_index);    (*src_index)++;
    tmp  =  *(src+*src_index) | *(src+*src_index+1) << 8;  // high byte last
    new_stop->offset  =  tmp;                *src_index +=  2;

    if ( head ) curr->next  = (struct MT_TABSTOP*) new_stop;
    else        head  =  new_stop;
    curr  =  new_stop;
    i++;
  }

  new_ruler->n_stops      =  num_stops;
  new_ruler->tabstop_list =  head;

  return new_ruler;
}


MT_FONT* Eqn_inputFONT( MTEquation* eqn,unsigned char* src,int* src_index ) {
  int zln;
  MT_FONT* new_font =  (MT_FONT*)malloc(sizeof(MT_FONT));

  (*src_index)++;            // step over the tag

  new_font->tface =  *(src+*src_index);    (*src_index)++;
  new_font->style =  *(src+*src_index);    (*src_index)++;

  zln =  strlen( (char*)src+*src_index ) + 1;
  new_font->zname =  (char*)malloc(sizeof(char)*zln);
  strcpy( (char*)new_font->zname,(char*)src+*src_index );
  *src_index +=  zln;

  return new_font;
}


MT_SIZE* Eqn_inputSIZE( MTEquation* eqn,unsigned char* src,int* src_index ) {

  MT_SIZE* new_size =  (MT_SIZE*)malloc(sizeof(MT_SIZE));
  int test;

  unsigned char size_tag =  *(src+*src_index) & 0x0F;
  (*src_index)++;

  if ( size_tag>=FULL && size_tag<=SUBSYM ) {
    new_size->type  =  size_tag;
    new_size->lsize =  size_tag - FULL;
    new_size->dsize =  0;

  } else {

    test =  *(src+*src_index);
    if        ( test==100 ) {
      new_size->type  =  100;                         (*src_index)++;
      new_size->lsize =  *(src+*src_index);           (*src_index)++;
      new_size->dsize =  *(src+*src_index);           (*src_index)++;
      new_size->dsize +=  ( *(src+*src_index) << 8 ); (*src_index)++;

    } else if ( test==101 ) {
      new_size->type  =  101;                         (*src_index)++;
      new_size->lsize =  *(src+*src_index);            (*src_index)++;

    } else {
      new_size->type  =  0;
      new_size->lsize =  *(src+*src_index);            (*src_index)++;
      new_size->dsize =  *(src+*src_index);            (*src_index)++;
    }
  }

  return new_size;
}


static
int GetNudge( unsigned char* src,int* x,int* y ) {

  int nudge_ln;
  short tmp;

  unsigned char b1 =  *src;
  unsigned char b2 =  *(src+1);

  if ( b1==128 && b2==128 ) {
    tmp  =  *(src+2) | *(src+3) << 8; // high byte last
    *x =  tmp;
    tmp  =  *(src+4) | *(src+5) << 8; // high byte last
    *y =  tmp;
    nudge_ln  =  6;
  } else {
    *x =  b1;
    *y =  b2;
    nudge_ln  =  2;
  }

  return nudge_ln;
}


// delete routines

static
void DeleteObjectList( MT_OBJLIST* the_list )
{
  MT_OBJLIST* del_node  =  the_list;
  MT_LINE* line;
  MT_CHAR* charptr;
  
  while ( the_list ) {

    del_node  =  the_list;
    the_list  =  (void *) the_list->next;

    switch ( del_node->tag ) {

      case LINE   : {
        line =  (MT_LINE*)del_node->obj_ptr;
        if ( line ) {
          if ( line->ruler ) {
            DeleteTabstops( line->ruler->tabstop_list );
            free(line->ruler);
          }

          if ( line->object_list )
            DeleteObjectList( line->object_list );
          free(line);
        }
      }
      break;

      case CHAR   : {
        charptr  =  (MT_CHAR*)del_node->obj_ptr;
        if ( charptr ) {
          if ( charptr->embellishment_list )
            DeleteEmbells( charptr->embellishment_list );
          free(charptr);
        }
      }
      break;

      case TMPL   : {
        MT_TMPL* tmpl =  (MT_TMPL*)del_node->obj_ptr;
        if ( tmpl ) {
          if ( tmpl->subobject_list )
            DeleteObjectList( tmpl->subobject_list );
          free(tmpl);
        }
      }
      break;

      case PILE   : {
        MT_PILE* pile =  (MT_PILE*)del_node->obj_ptr;
        if ( pile ) {
          if ( pile->line_list )
            DeleteObjectList( pile->line_list );
          free(pile);
        }
      }
      break;

      case MATRIX : {
        MT_MATRIX* matrix =  (MT_MATRIX*)del_node->obj_ptr;
        if ( matrix ) {
          if ( matrix->element_list )
            DeleteObjectList( matrix->element_list );
          free(matrix);
        }
      }
      break;

      case EMBELL :
      break;

      case RULER  : {
        MT_RULER* ruler =  (MT_RULER*)del_node->obj_ptr;
        if ( ruler ) {
          if ( ruler->tabstop_list )
            DeleteTabstops( ruler->tabstop_list );
          free(ruler);
        }
      }
      break;

      case FONT   : {
        MT_FONT* font =  (MT_FONT*)del_node->obj_ptr;
        if ( font ) {
          if ( font->zname )
            free(font->zname);
          free(font);
        }
      }
      break;

      case SIZE   :
      case FULL   :
      case SUB    :
      case SUB2   :
      case SYM    :
      case SUBSYM :  {
        MT_SIZE* size =  (MT_SIZE*)del_node->obj_ptr;
        if ( size ) {
          free(size);
        }
      }
      break;

      default :
      break;
    }

    free(del_node);
  }     //  while ( the_list )
}


static
void DeleteTabstops( MT_TABSTOP* the_list )
{
  MT_TABSTOP* del_node;

  while ( the_list ) {
    del_node  =  the_list;
    the_list  =  (MT_TABSTOP*) (the_list->next);
    free(del_node);
  }
}


static
void DeleteEmbells( MT_EMBELL* the_list )
{
  MT_EMBELL* del_node;
  while ( the_list ) {
    del_node  =  the_list;
    the_list  =  (MT_EMBELL*)the_list->next;
    free(del_node);
  }
}


// formatting routines.  convert internal form to LaTeX
static
char* Eqn_TranslateObjects( MTEquation* eqn,MT_OBJLIST* the_list ) {

  char* zcurr;
  char* rv  =  (char*)malloc(1024);
  int   di  =  0;
  int   lim =  1024;

  MT_OBJLIST* curr_node;
  *rv       =  0;

  while ( the_list ) {

    curr_node =  the_list;
    the_list  =  (void *) the_list->next;

    zcurr =  (char*)NULL;

    switch ( curr_node->tag ) {

      case LINE   : {
        MT_LINE* line =  (MT_LINE*)curr_node->obj_ptr;
        if ( line )
          zcurr =  Eqn_TranslateLINE( eqn,line );
      }
      break;

      case CHAR   : {
        MT_CHAR* charptr  =  (MT_CHAR*)curr_node->obj_ptr;
        if ( charptr ) {
          int advance =  0;
          if ( charptr->typeface==130 )  {    // auto_recognize functions
            zcurr =  Eqn_TranslateFUNCTION( eqn,curr_node,&advance );
            while ( advance>1 ) {
              the_list  =  (MT_OBJLIST *) the_list->next;
              advance--;
            }
          } else if ( charptr->typeface==129 && eqn->text_mode ) { // text in math
            zcurr =  Eqn_TranslateTEXTRUN( eqn,curr_node,&advance );
            while ( advance>1 ) {
              the_list  =  (MT_OBJLIST *) the_list->next;
              advance--;
            }
          }
          if ( !advance )
            zcurr =  Eqn_TranslateCHAR( eqn,charptr );
        }
      }
      break;

      case TMPL   : {
        MT_TMPL* tmpl =  (MT_TMPL*)curr_node->obj_ptr;
        if ( tmpl )
          zcurr =  Eqn_TranslateTMPL( eqn,tmpl );
      }
      break;

      case PILE   : {
        MT_PILE* pile =  (MT_PILE*)curr_node->obj_ptr;
        if ( pile )
          zcurr =  Eqn_TranslatePILE( eqn,pile );
      }
      break;

      case MATRIX : {
        MT_MATRIX* matrix =  (MT_MATRIX*)curr_node->obj_ptr;
        if ( matrix )
          zcurr =  Eqn_TranslateMATRIX( eqn,matrix );
      }
      break;

      case EMBELL :
      break;

      case RULER  : {
        MT_RULER* ruler =  (MT_RULER*)curr_node->obj_ptr;
        if ( ruler )
          zcurr =  Eqn_TranslateRULER( eqn,ruler );
      }
      break;

      case FONT   : {
        MT_FONT* font =  (MT_FONT*)curr_node->obj_ptr;
        if ( font )
          zcurr =  Eqn_TranslateFONT( eqn,font );
      }
      break;

      case SIZE   :
      case FULL   :
      case SUB    :
      case SUB2   :
      case SYM    :
      case SUBSYM :  {
        MT_SIZE* size =  (MT_SIZE*)curr_node->obj_ptr;
        if ( size )
          zcurr =  Eqn_TranslateSIZE( eqn,size );
      }
      break;

      default :
      break;
    }

    if ( zcurr )
      rv  =  ToBuffer( zcurr,rv,&di,&lim );

  }     //  while ( the_list )

  return rv;
}

static
char* Eqn_TranslateLINE( MTEquation* eqn,MT_LINE* line ) {

  EQ_STRREC strs[3];

  int num_strs  =  0;
  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sLINE\n",eqn->indent );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strcat( eqn->indent,"  " );

    if ( line->ruler ) {
      strs[num_strs].log_level   =  0;
      strs[num_strs].do_delete   =  1;
      strs[num_strs].ilk         =  Z_TEX;
      strs[num_strs].is_line     =  0;
      strs[num_strs].data        =  Eqn_TranslateRULER( eqn,line->ruler );
      num_strs++;
    }

    if ( line->object_list ) {
      strs[num_strs].log_level  =  0;
      strs[num_strs].do_delete  =  1;
      strs[num_strs].ilk        =  Z_TEX;
      strs[num_strs].is_line    =  0;
      strs[num_strs].data       =  Eqn_TranslateObjects( eqn,line->object_list );
      num_strs++;
    }

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslateCHAR( MTEquation* eqn,MT_CHAR* thechar ) {

  EQ_STRREC strs[4];
  int save_index;
  int num_strs  =  0;
  int math_attr;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sCHAR : atts=%d,typeface=%d,char=%c,%ld\n",eqn->indent,
                  thechar->atts,thechar->typeface,
                  (char) thechar->character,thechar->character );
    SetComment( strs,2,buf );
    num_strs++;
  }

  SetComment( strs+num_strs,100,(char*)NULL );  // place_holder for $ if needed
  save_index  =  num_strs++;

  strcat( eqn->indent,"  " );

  num_strs  +=  Eqn_GetTexChar( eqn,strs+num_strs,thechar,&math_attr );

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  if ( math_attr ) {
    if ( eqn->math_mode==0 ) {
      if ( math_attr==MA_FORCE_MATH ) {
        SetDollar( strs+save_index,1 );       // turn math on
        eqn->math_mode =  1;
      }
    } else if ( eqn->math_mode==1 ) {
      if ( math_attr==MA_FORCE_TEXT ) {
        SetDollar( strs+save_index,0 );       // turn math off
        eqn->math_mode =  0;
      }
    } else {

    }
  }

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslateFUNCTION( MTEquation* eqn,MT_OBJLIST* curr_node,int* advance ) {

  // Gather the function name

  int   di  =  0;
  char  nom[16];
  char tex_func[128];
  EQ_STRREC strs[4];
  int num_strs  =  0;
  int zlen;
  int save_index;
  char* zdata;

  *advance =  0;
  
  while ( curr_node && curr_node->tag==CHAR ) {
    MT_CHAR* charptr  =  (MT_CHAR*)curr_node->obj_ptr;
    if ( charptr && charptr->typeface==130 && isalpha(charptr->character) ) {
      nom[di++] =  (char)charptr->character;
      curr_node =  (MT_OBJLIST*) curr_node->next;
      (*advance)++;
    } else
      break;
  }

  nom[di] =  0;
  
  /* di check added by Ujwal S. Sathyam */
  if (!di) { 
    *advance =  0;
    return (char*)NULL;
  }
  

  // get the translation for this function from the INI file

  zlen  =  GetProfileStr( Profile_FUNCTIONS,nom,tex_func,128 );

  if ( !zlen || !tex_func[0]) { 
    *advance =  0;
    return (char*)NULL;
  }

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sFUNCTION : %s\n",eqn->indent,nom );
    SetComment( strs,2,buf );
   num_strs++;
  }

  SetComment( strs+num_strs,100,(char*)NULL );  // place_holder for $ if needed
  save_index  =  num_strs++;

  strs[num_strs].log_level   =  0;
  strs[num_strs].do_delete   =  1;
  strs[num_strs].ilk         =  Z_TEX;
  strs[num_strs].is_line     =  0;
  zdata =  (char*)malloc(strlen(tex_func)+1);
  strcpy( zdata,tex_func );
  strs[num_strs].data        =  zdata;
  num_strs++;

  if ( *advance && eqn->math_mode==0 ) {
    SetDollar( strs+save_index,1 );          // turn math on
    eqn->math_mode =  1;
  }

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslateTEXTRUN( MTEquation* eqn,MT_OBJLIST* curr_node,int* advance )
{
  // Gather the tex run

  int   di  =  0;
  char  run[256];
  EQ_STRREC strs[4];
  int num_strs  =  0;
  char tbuff[256];
  int zlen;
  char* zdata;
  char* ndl;

  *advance =  0;
  
  while ( curr_node && ( curr_node->tag==CHAR || curr_node->tag==SIZE ) ) {
    if ( curr_node->tag==CHAR ) {
      MT_CHAR* charptr  =  (MT_CHAR*)curr_node->obj_ptr;
      if ( charptr && charptr->typeface==129 )  {
        run[di++] =  (char)charptr->character;
        curr_node =  (MT_OBJLIST*) curr_node->next;
        (*advance)++;
      } else
        break;
    } else {
      curr_node = (MT_OBJLIST*) curr_node->next;
      (*advance)++;
    }
  }
  run[di] =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[256];
    sprintf( buf,"\n%sTEXTRUN : %s\n",eqn->indent,run );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strs[num_strs].log_level   =  0;
  strs[num_strs].do_delete   =  1;
  strs[num_strs].ilk         =  Z_TEX;
  strs[num_strs].is_line     =  0;

  zlen  =  GetProfileStr( Profile_TEMPLATES,"TextInMath",tbuff,256 );
  if ( zlen<=0 )
    strcpy( tbuff,"\\text{#1}" );

  zdata =  malloc(di+zlen);

  ndl =  strchr( tbuff,'#' );
  if ( ndl ) {
    *ndl  =  0;
    strcpy( zdata,tbuff );
  } else
    *zdata  =  0;
  strcat( zdata,run );
  if ( ndl ) {
    ndl +=  2;
    strcat( zdata,ndl );
  }

  strs[num_strs].data        =  zdata;
  num_strs++;

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}

static
int HasHVLine( int line_num,unsigned char* bits ) {

  int rv  =  0;
  int byte_num  =  line_num / 4;
  int shift     =  ( line_num % 4 ) * 2;
  int mask      =  0x03 << shift;

  if ( byte_num<MATR_MAX )
    rv  =  ( bits[byte_num] & mask ) ? 1 : 0;

  return rv;
}


// WARNING: hard coded translation
static
char* Eqn_TranslateMATRIX( MTEquation* eqn,MT_MATRIX* matrix ) {

  int buf_limit =  8192;
  char* rv  =  malloc(buf_limit);
  int is_tabular;
  char* matv_align  =  "{";           // default is vertical centering
  char* col_align =  "c";
  int i =  0;
  MT_OBJLIST* obj_list;
  int curr_row  =  0;
  int curr_col  =  0;

  *rv =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sStart MATRIX\n",eqn->indent );
    strcat( rv,buf );
  }

  strcat( eqn->indent,"  " );

  // put "\n\\begin{array}"

    if ( eqn->math_mode ) {
      strcat( rv,"\n\\begin{array}" );
      eqn->math_mode++;
      is_tabular  =  0;
    } else {
//      strcat( rv,"\n$\n\\begin{array}" );
//      math_mode =  2;
      strcat( rv,"\n\\begin{tabular}" );
      is_tabular  =  1;
    }

  // set the vertical alignment of the matrix

    if      ( matrix->valign==0 )
      matv_align  =  "[t]{";
    else if ( matrix->valign==2 )
      matv_align  =  "[b]{";
    strcat( rv,matv_align );

  // set the horizontal column alignment char

    if      ( matrix->h_just==MT_LEFT )
      col_align =  "l";
    else if ( matrix->h_just==MT_RIGHT )
      col_align =  "r";

  // script the LaTeX "cols"

    while ( i<=matrix->cols ) {
      if ( HasHVLine(i,matrix->col_parts) )
        strcat( rv,"|" );
      if ( i<matrix->cols )
        strcat( rv,col_align );
      i++;
    }
    strcat( rv,"}\n" );

    if ( !is_tabular ) eqn->text_mode++;
    obj_list =  matrix->element_list;

    while ( obj_list && curr_row<matrix->rows ) {     //  loop down rows

      int new_line  =  0;
      if ( curr_row ) {
        if ( is_tabular && eqn->math_mode ) {
          strcat( rv," $ \\\\" );
          eqn->math_mode =  0;
        } else
          strcat( rv," \\\\" );
        new_line  =  1; 
      }
      if ( HasHVLine(curr_row,matrix->row_parts) ) {
        strcat( rv," \\hline" );
        new_line  =  1; 
      }
      if ( new_line )   strcat( rv,"\n" );

      while ( curr_col<matrix->cols ) {     // loop thru columns in one row

        if ( curr_col ) {
          if ( is_tabular && eqn->math_mode ) {
            strcat( rv," $ & " );
            eqn->math_mode =  0;
          } else
            strcat( rv," & " );
        }

        while ( obj_list && obj_list->tag!=LINE ) {     // could be a SIZE
          obj_list  =  (MT_OBJLIST*) obj_list->next;
        }

        if ( obj_list && obj_list->tag==LINE ) {
          char* data  =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );

          int b_off =  strlen( rv );
          rv  =  ToBuffer( data,rv,&b_off,&buf_limit );    // strcat( rv,data );

          curr_col++;
          obj_list  =  (MT_OBJLIST*) obj_list->next;
        } else
          break;

      }   // loop thru columns in one row

      curr_row++;

    }               // loop down rows
  
    if ( is_tabular && eqn->math_mode ) {
      strcat( rv," $" );
      eqn->math_mode =  0;
    }

    if ( HasHVLine(curr_row,matrix->row_parts) )
      strcat( rv," \\\\ \\hline\n" );

  // put "\n\\end{array}\n"

    if ( is_tabular )
      strcat( rv,"\n\\end{tabular}\n" );
    else
      strcat( rv,"\n\\end{array}\n" );

    if ( !is_tabular ) eqn->text_mode--;

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sEnd MATRIX\n",eqn->indent );
    strcat( rv,buf );
  }

  if ( !is_tabular )  eqn->math_mode--;

  return rv;
}

static
int Is_RelOp( MT_CHAR* charptr ) {

  int rv  =  0;

  if ( charptr->typeface==134 ) {
  
    if ( charptr->character==60 || charptr->character==62
    || charptr->character==163 || charptr->character==179
    || charptr->character==185 || charptr->character==186
    || charptr->character==187 || charptr->character==64
    || charptr->character==61 || charptr->character==181 )
      rv  =  1;    
        
  } else if ( charptr->typeface==139 ) {
    if ( charptr->character==112 || charptr->character==102
    || charptr->character==60 || charptr->character==62 )
      rv  =  1;
  }

  return rv;
}


static
char* Eqn_TranslateEQNARRAY( MTEquation* eqn,MT_PILE* pile ) {

  int buf_limit =  8192;
  char* rv  =  (char*)malloc(buf_limit);
  int save_math_mode;
  MT_OBJLIST* obj_list;
  int curr_row  =  0;
  int right_only  =  0;
  char* data;
  int b_off;

  *rv =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sStart EQNARRAY\n",eqn->indent );
    strcat( rv,buf );
  }

  strcat( eqn->indent,"  " );

  // put "\n\\begin{eqnarray}"

  save_math_mode  =  eqn->math_mode;

    if ( eqn->math_mode ) {
      strcpy( rv," $\n\\begin{eqnarray*}\n" );
    } else {
      strcpy( rv,"\\begin{eqnarray*}\n" );
    }
    eqn->math_mode =  2;
    eqn->text_mode++;

    obj_list  =  pile->line_list;

    while ( obj_list ) {      //  loop down lines

      if ( curr_row )
        strcat( rv," \\\\\n" );

      while ( obj_list && obj_list->tag!=LINE ) {     // could be a SIZE
        obj_list  =  (MT_OBJLIST*) obj_list->next;
      }

      if ( obj_list && obj_list->tag==LINE ) {

// locate the "relop" in the object_list of the current line

        MT_OBJLIST* left_node   =  (MT_OBJLIST*)NULL;
        MT_OBJLIST* right_node  =  (MT_OBJLIST*)NULL;

        MT_LINE* line =  (MT_LINE*)obj_list->obj_ptr;
        MT_OBJLIST* curr_node =  line->object_list;
        while ( curr_node ) {
          if ( curr_node->tag==CHAR ) {    // check for a relop
            if ( Is_RelOp((MT_CHAR*)curr_node->obj_ptr) )
              break;
          }
          left_node =  curr_node;
          curr_node =  (MT_OBJLIST*) curr_node->next;
        }

// handle left side

        if ( left_node ) {
          if ( curr_node ) {
            left_node->next =  NULL;
            data  =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );

            b_off =  strlen( rv );
            rv  =  ToBuffer( data,rv,&b_off,&buf_limit );    // strcat( rv,data );

            left_node->next =  (void*) curr_node;
          } else {
            right_node  =  left_node;
            right_only  =  1;
          }
        }
        strcat( rv," & " );

// handle the relop

        if ( curr_node ) {
          char* data  =  Eqn_TranslateCHAR( eqn,(MT_CHAR*)curr_node->obj_ptr );
          strcat( rv,data );
          free(data);
          right_node  = (MT_OBJLIST*) curr_node->next;
        }
        strcat( rv," & " );

// handle right side

        if ( right_node ) {
          char* data;
          int b_off;
          if ( right_only )
            data  =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );
          else
            data  =  Eqn_TranslateObjects( eqn,right_node );

          b_off =  strlen( rv );
          rv  =  ToBuffer( data,rv,&b_off,&buf_limit );    // strcat( rv,data );
        }


        obj_list  = (MT_OBJLIST*)  obj_list->next;
      } else
        break;

      curr_row++;
    }                       // loop down thru lines
  

  // put "\n\\end{array}\n"

    strcat( rv,"\n\\end{eqnarray*}\n" );

    if ( save_math_mode )
      strcat( rv,"$ " );
    eqn->math_mode =  save_math_mode;

    eqn->text_mode--;

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"%sEnd EQNARRAY\n",eqn->indent );
    strcat( rv,buf );
  }

  return rv;
}


static
char* Eqn_TranslateTABULAR( MTEquation* eqn,MT_PILE* pile ) {

  int buf_limit =  8192;
  char* rv  =  malloc(buf_limit);
  int save_math_mode  =  eqn->math_mode;
  char* tablev_align  =  "{";           // default is vertical centering
  char* col_align =  "c";
  MT_OBJLIST* obj_list  =  pile->line_list;
  int curr_row  =  0;

  *rv =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sStart TABULAR\n",eqn->indent );
    strcat( rv,buf );
  }

  strcat( eqn->indent,"  " );

  // put "\n\\begin{tabular}"

    if ( eqn->math_mode ) {
      strcpy( rv," $\n\\begin{tabular}" );
      eqn->math_mode =  0;
    } else {
      strcpy( rv,"\n\\begin{tabular}" );
    }

  // set the vertical alignment of the tabular

    if      ( pile->valign==0 )
      tablev_align  =  "[t]{";
    else if ( pile->valign==2 )
      tablev_align  =  "[b]{";
    strcat( rv,tablev_align );

  // set the horizontal column alignment char

    if      ( pile->valign==MT_PILE_LEFT )
      col_align =  "l";
    else if ( pile->valign==MT_PILE_RIGHT )
      col_align =  "r";

  // script the LaTeX "cols"

    strcat( rv,col_align );

    strcat( rv,"}\n" );

    obj_list  =  pile->line_list;

    curr_row  =  0;
    while ( obj_list ) {          //  loop down thru lines

      if ( curr_row ) {
        if ( eqn->math_mode ) {
          strcat( rv," $ \\\\\n" );
          eqn->math_mode =  0;
        } else
          strcat( rv," \\\\\n" );
      }

      while ( obj_list && obj_list->tag!=LINE ) {     // could be a SIZE
        obj_list  =  (MT_OBJLIST*) obj_list->next;
      }

      if ( obj_list && obj_list->tag==LINE ) {
        char* data  =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );

        int b_off =  strlen( rv );
        rv  =  ToBuffer( data,rv,&b_off,&buf_limit );    // strcat( rv,data );

        obj_list  =  (MT_OBJLIST*) obj_list->next;
      } else
        break;

      curr_row++;
    }                           // loop down thru lines
  

  // put "\n\\end{tabular}\n"

    if ( eqn->math_mode ) {
      strcat( rv," $" );
      eqn->math_mode =  0;
    }
    strcat( rv,"\n\\end{tabular}\n" );

    if ( save_math_mode ) {
      strcat( rv,"$ " );
      eqn->math_mode =  save_math_mode;
    }

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sEnd TABULAR\n",eqn->indent );
    strcat( rv,buf );
  }

  return rv;
}


static
char* Eqn_TranslatePILEtoTARGET( MTEquation* eqn,MT_PILE* pile,char* targ_nom ) {

  char ini_line[256];
  int dlen  =  0;

  int forces_math =  1;
  int forces_text =  0;
  int allow_text_runs =  1;
  char* head  =  "";
  char* line_sep  =  " \\\\ ";
  char* tail  =  "";
  int buf_limit =  8192;
  char* rv  =  (char*)malloc(buf_limit);
  int save_math_mode;
  MT_OBJLIST* obj_list;
  int curr_row  =  0;


  *rv =  0;

  if ( targ_nom && *targ_nom )
    dlen  =  GetProfileStr( Profile_PILEtranslation,targ_nom,ini_line,256 );


  //  ini_line  =  "TextOnly,\begin{env}, \\,\end{env}"

  if ( dlen ) {
    char* rover =  ini_line;
    if ( *rover=='T' ) {
      forces_math =  0;
      forces_text =  1;
      allow_text_runs =  0;
    }
    rover =  strchr( rover,',' );             // end math/text force flag
    if ( rover && *(rover+1) ) {
      rover++;                                // start of head
      if ( *rover!=',' ) head =  rover;
      rover =  strchr( rover,',' );           // end of head
      if ( rover ) {
        *rover  =  0;
        rover++;                              // start of line_sep
        if ( *rover && *(rover+1) ) {
          if ( *rover!=',' ) line_sep =  rover;
          rover =  strchr( rover,',' );       // end of line_sep
          if ( rover ) {
            *rover  =  0;
            rover++;                          // start of tail
            if ( *rover )
              if ( *rover!=',' ) tail =  rover;
          }
        }
      }
    }
  }

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sStart PILE in TMPL field\n",eqn->indent );
    strcat( rv,buf );
  }

  strcat( eqn->indent,"  " );

  if ( forces_math ) {
    if ( eqn->math_mode==0 ) {
      strcat( rv," $" );
      eqn->math_mode =  1;
    }
    eqn->math_mode++;

  } else if ( forces_text ) {
    if ( eqn->math_mode ) {
      save_math_mode  =  eqn->math_mode;
      strcat( rv," $" );
      eqn->math_mode =  0;
    }
  }

  if ( allow_text_runs )
    eqn->text_mode++;

  // put "\\begin{array}"

  strcat( rv,"\n" );
  strcat( rv,head );
  strcat( rv,"\n" );


    obj_list  =  pile->line_list;

    while ( obj_list ) {          //  loop down thru lines

      if ( curr_row ) {
        if ( forces_text && eqn->math_mode ) {
          strcat( rv," $" );
          eqn->math_mode =  0;
        }
        strcat( rv,line_sep );
        strcat( rv,"\n" );
      }

      while ( obj_list && obj_list->tag!=LINE ) {     // could be a SIZE
        obj_list  =  (MT_OBJLIST*) obj_list->next;
      }

      if ( obj_list && obj_list->tag==LINE ) {
        char* data  =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );

        int b_off =  strlen( rv );
        rv  =  ToBuffer( data,rv,&b_off,&buf_limit );

        obj_list  = (MT_OBJLIST*)  obj_list->next;
      } else
        break;

      curr_row++;
    }                             // loop down thru lines
  
  if ( allow_text_runs )
    eqn->text_mode--;

  // put "\n\\end{array*}\n"

  if ( forces_text && eqn->math_mode ) {
    strcat( rv," $" );
    eqn->math_mode =  0;
  }
  strcat( rv,"\n" );
  strcat( rv,tail );
  strcat( rv,"\n" );

  if ( forces_math ) {
    eqn->math_mode--;
  } else if ( forces_text ) {
    if ( save_math_mode ) {
      strcat( rv,"$ " );
      eqn->math_mode =  save_math_mode;
    }
  }

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sEnd PILE in TMPL field\n",eqn->indent );
    strcat( rv,buf );
  }

  return rv;
}


static
char* Eqn_TranslateSIZE( MTEquation* eqn,MT_SIZE* size ) {

  EQ_STRREC strs[2];

  int num_strs  =  0;
  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sSIZE\n",eqn->indent );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strcat( eqn->indent,"  " );

  // not translation implemented yet

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslateRULER( MTEquation* eqn,MT_RULER* ruler ) {

  EQ_STRREC strs[2];
  int num_strs  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sRULER\n",eqn->indent );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strcat( eqn->indent,"  " );

  // no translation implemented yet

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslateFONT( MTEquation* eqn,MT_FONT* font ) {

  EQ_STRREC strs[2];
  int num_strs  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sFONT\n",eqn->indent );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strcat( eqn->indent,"  " );

  // no translation implemented yet

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  return  Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* ToBuffer( char* src,char* buffer,int* off,int* lim ) {

  int zln =  strlen(src) + 1;

  if ( *off+zln+256 >= *lim ) {
    char* newbuf;
    *lim =  *off+zln+1024;
    newbuf  =  (char*)malloc(*lim);
    strcpy( newbuf,buffer );
    free(buffer);
    buffer  =  newbuf;
  }

  strcpy( buffer+*off,src );
  *off +=  zln-1;
  free(src);
  
  return buffer;
}

static
void SetComment( EQ_STRREC* strs,int lev,char* src )
{
  strs[0].log_level   =  lev;
  strs[0].do_delete   =  1;
  strs[0].ilk         =  Z_COMMENT;
  strs[0].is_line     =  0;

  if ( src ) {
    int zln =  strlen(src) + 1;
    char* newbuf  =  (char*)malloc(zln);
    strcpy( newbuf,src );
    strs[0].data  =  newbuf;
  } else
    strs[0].data  =  (char*)NULL;
}


static
void SetDollar( EQ_STRREC* strs,int turn_on )
{
  strs[0].log_level   =  0;
  strs[0].do_delete   =  0;
  strs[0].ilk         =  Z_TEX;
  strs[0].is_line     =  0;
//  strs[0].data        =  turn_on ? " $" : "$ "; /* spaces removed by Ujwal S. Sathyam */
  strs[0].data        =  turn_on ? "$" : "$";
}


static
char* Eqn_JoinzStrs( MTEquation* eqn,EQ_STRREC* strs,int num_strs )
{
  char* join;

  int zln  =  0;

  int count =  0;
  while ( count<num_strs ) {
    int   lev =  strs[count].log_level;
    char* dat =  strs[count].data;
    if ( dat )
      if ( lev<=eqn->log_level )
        zln +=  strlen( dat );
    count++;
  }

  join  =  (char*)malloc(zln+1);
  *join =  0;

  count =  0;
  while ( count<num_strs ) {
    int   lev =  strs[count].log_level;
    char* dat =  strs[count].data;
    if ( dat ) {
      if ( lev<=eqn->log_level ) {
        if ( strs[count].ilk==Z_TMPL ) {    // dat = "\sqrt[#2]{#1}"
          int   si  =  0;
          int   di  =  strlen( join );
          char  ch;
          while ( (ch = dat[si]) ) {
            if        ( ch=='\\' ) {
              join[di++]  =  ch;
              si++;
              if ( (ch = dat[si]) )   join[di++]  =  ch;
              else                  break;
            } else if ( ch=='#' ) {
              si++;
              if ( (ch = dat[si]) ) {   // 1 - 9

                char* vars[9];
                int var_count =  0;
                char* thetex;
                int slot  =  count + 1;
                int curr_line_num;
                int targ_line_num;

                while ( var_count<9 ) vars[ var_count++ ] =  (char*)NULL;

                var_count =  0;
                while ( dat[si+1]=='[' ) {
// Get the [] args
                  char* nom_end =  strchr( dat+si+2,']' );
                  if ( nom_end ) {
                    *nom_end  =  0;
                    vars[ var_count++ ] =  dat + si + 2;
                    si  +=  strlen( dat+si+2 ) + 2;
                  } else
                    break;
                }

// Search strs beyond count for the appropriate LINE

                thetex  =  (char*)NULL;
                slot  =  count + 1;
                curr_line_num  =  0;
                targ_line_num  =  ch - '1';
                while ( slot<num_strs ) {
                  if ( strs[slot].is_line ) {
                    if ( curr_line_num==targ_line_num ) {
                      thetex  =  strs[slot].data;
                      break;
                    } else
                      curr_line_num++;
                  }
                  slot++;
                }

                if ( thetex ) {

                  char* var_nom =  vars[1];
                  if ( var_nom ) {              // we have a variable leadin
                    char buf[128];
                    int zdl =  GetProfileStr( Profile_VARIABLES,var_nom,buf,128 );
                    if ( zdl>0 )  {
                      char* head  =  buf;
                      char* comma =  strchr( buf,',' );
                      int inc_len;  
                      if ( strs[slot].is_line==2 )        // it's a pile
                        head  =  comma + 1;
                      else
                        *comma  =  0;
                      inc_len =  strlen( head );  
                      strcpy( join+di,head );
                      di  +=  inc_len;                    
                    }
                  }

                  strcpy( join+di,thetex );
                  di  +=  strlen( thetex );

                  var_nom =  vars[2];
                  if ( var_nom ) {
                    char buf[128];
                    int zdl =  GetProfileStr( Profile_VARIABLES,var_nom,buf,128 );
                    if ( zdl>0 )  {
                      char* tail  =  buf;
                      char* comma =  strchr( tail,',' );
                      int inc_len;  
                      if ( strs[slot].is_line==2 )        // it's a pile
                        tail  =  comma + 1;
                      else
                        *comma  =  0;
                      inc_len =  strlen( tail );  
                      strcpy( join+di,tail );
                      di  +=  inc_len;                    
                    }
                  }

                  strs[slot].log_level  =  100;     // cancel this node
                }
              } else
                break;

            } else
              join[di++]  =  ch;
            si++;
          }
          join[di]  =  0;
        } else
          strcat( join,dat );
      }
      if ( strs[count].do_delete )
        free(dat);
    }
    count++;
  }

  return join;
}

static
char* Eqn_TranslateTMPL( MTEquation* eqn,MT_TMPL* tmpl )
{
  EQ_STRREC strs[10];
  char* the_template;
  int tally =  1;      
  int num_strs  =  0;
  MT_OBJLIST* obj_list;


  if ( eqn->math_mode==0 ) {
    SetDollar( strs,1 );
    num_strs++;
    eqn->math_mode =  1;
  }
  eqn->math_mode++;

  num_strs  +=  Eqn_GetTmplStr( eqn,tmpl->selector,tmpl->variation,strs+num_strs );

  the_template  =  strs[num_strs-1].data;

  strcat( eqn->indent,"  " );

    obj_list  =  tmpl->subobject_list;

    if ( obj_list && obj_list->tag==CHAR ) {

//   translateCHAR( (MT_CHAR*)obj_list->obj_ptr );

      obj_list  = (MT_OBJLIST*)  obj_list->next;
    }
  
    if ( obj_list && obj_list->tag>=SIZE && obj_list->tag<=SUBSYM ) {
//
      obj_list  = (MT_OBJLIST*)  obj_list->next;
    }
  
    while ( obj_list ) {
      if ( obj_list->tag==LINE ) {
        strs[num_strs].log_level  =  0;
        strs[num_strs].do_delete  =  1;
        strs[num_strs].ilk        =  Z_TEX;
        strs[num_strs].is_line    =  1;
        strs[num_strs].data       =  Eqn_TranslateLINE( eqn,(MT_LINE*)obj_list->obj_ptr );
        num_strs++;
        tally++;
      } else if ( obj_list->tag==PILE ) {      // This one is DIFFICULT!!
        char targ_nom[32];
        strs[num_strs].log_level  =  0;
        strs[num_strs].do_delete  =  1;
        strs[num_strs].ilk        =  Z_TEX;
        strs[num_strs].is_line    =  2;

        GetPileType( the_template,tally,targ_nom );

        strs[num_strs].data =  Eqn_TranslatePILEtoTARGET( eqn,
                                (MT_PILE*)obj_list->obj_ptr,targ_nom );

        num_strs++;
        tally++;
      }

      obj_list  = (MT_OBJLIST*)  obj_list->next;
    }

    //There may be a SIZE at the end of the list

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  eqn->math_mode--;
  return Eqn_JoinzStrs( eqn,strs,num_strs );
}


static
char* Eqn_TranslatePILE( MTEquation* eqn,MT_PILE* pile )
{
  EQ_STRREC strs[2];
  int num_strs  =  0;

  if ( eqn->log_level>=2 ) {  
    char buf[128];
    sprintf( buf,"\n%sPILE\n",eqn->indent );
    SetComment( strs,2,buf );
    num_strs++;
  }

  strcat( eqn->indent,"  " );

    strs[num_strs].log_level   =  0;
    strs[num_strs].do_delete   =  1;
    strs[num_strs].ilk         =  Z_TEX;
    strs[num_strs].is_line     =  0;

    if ( pile->halign==MT_PILE_OPERATOR )
      strs[num_strs].data  =  Eqn_TranslateEQNARRAY( eqn,pile );
    else
      strs[num_strs].data  =  Eqn_TranslateTABULAR( eqn,pile );
    num_strs++;

  eqn->indent[ strlen(eqn->indent)-2 ]  =  0;

  return Eqn_JoinzStrs( eqn,strs,num_strs );
}

// Character translation, MathType to TeX, using inifile data

static
int Eqn_GetTexChar( MTEquation* eqn,EQ_STRREC* strs,MT_CHAR* thechar,int* math_attr )
{
  int num_strs    =  0;     // this holds the returned value

  int set         =  thechar->typeface;
  int code_point  =  thechar->character;

  char* ztex  =  (char*)NULL;
  char  zch   =  0;
  char* zdata =  (char*)NULL;

  CHARSETatts set_atts;

  if ( set >= 129 && set < 129+NUM_TYPEFACE_SLOTS ) {
    set_atts  =  eqn->atts_table[set-129];
  } else {                                // unexpected charset
    char buff[16];
    char key[16];
    int zln;
    sprintf( key, "%d", set );
    zln =  GetProfileStr( eqn->m_atts_table,key,buff,16 );
    if ( zln ) {
      set_atts.mathattr       =  buff[0] - '0';
      set_atts.do_lookup      =  buff[2] - '0';
      set_atts.use_codepoint  =  buff[4] - '0';
    } else {
      set_atts.mathattr       =  1;
      set_atts.do_lookup      =  1;
      set_atts.use_codepoint  =  1;
    }
  }

  *math_attr  =  set_atts.mathattr;
  if ( set_atts.do_lookup ) {
    int zln;
    char key[16];                     // 132.65m
    char buff[256];

    sprintf( key, "%d.%d", set, code_point );

    if ( *math_attr==3 ) {
      if ( eqn->math_mode && !eqn->text_mode )  strcat( key,"m" );
      else                            strcat( key,"t" );
      *math_attr =  0;
    }

    zln =  GetProfileStr( eqn->m_char_table,key,buff,256 );
    if ( zln ) {
      ztex  =  (char*)malloc(zln+1);
      strcpy( ztex,buff );
    }
  }

  if ( !ztex && set_atts.use_codepoint )
    if ( code_point>=32 && code_point <=127 )
      zch   =  code_point;

  if ( ztex ) {
    zdata =  ztex;
  } else if ( zch ) {
    zdata     =  (char*)malloc(2);
    zdata[0]  =  zch;
    zdata[1]  =  0;
  }

  if ( zdata ) {

    MT_EMBELL*  embells =  thechar->embellishment_list;
    while ( embells ) {
      char tmp[128];
      int zlen;
      strcpy( tmp,Template_EMBELLS[embells->embell] );
      zlen = strlen(tmp);

      if ( zlen>0 ) {   // patch the char into the embell template

        int   si  =  0;
        char* ptr =  strchr( tmp,',' );
        char* join;
        int   di  =  0;
        char  ch;
        if ( ptr ) {
          if ( eqn->math_mode || *math_attr==MA_FORCE_MATH )  *ptr  =  0;
          else              si    =  ptr - tmp + 1;
          zlen  =  strlen( tmp+si );
        }

        join  =  (char*)malloc(zlen + strlen(zdata) + 16);
        while ( (ch = tmp[si]) ) {
          if        ( ch=='\\' ) {
            join[di++]  =  ch;
            si++;
            if ( (ch = tmp[si]) )   join[di++]  =  ch;
            else                  break;
          } else if ( ch=='%' ) {
            si++;
            if ( (ch = tmp[si]) ) {   // 1 - 9
              strcpy( join+di,zdata );
              free(zdata);
              di  =  strlen( join );
            } else
              break;
          } else
            join[di++]  =  ch;
          si++;
        }

        join[di]  =  0;
        zdata =  join;

      }       // patch char into embell template

      embells   = (MT_EMBELL *) embells->next;

    }     // while ( embells )

    strs[0].log_level   =  0;
    strs[0].do_delete   =  1;
    strs[0].ilk         =  Z_TEX;
    strs[0].is_line     =  0;
    strs[0].data        =  zdata;
    num_strs++;

  }   //  if ( zdata )

  return num_strs;
}


static
int Eqn_GetTmplStr( MTEquation* eqn,int selector,int variation,EQ_STRREC* strs )
{
  char key[8];                      // key = "20.1"
  char ini_line[256];
  char* tmpl_ptr;
  int num_strs  =  0;           // this becomes the return value
  int zlen;

  sprintf( key, "%d.%d", selector, variation ); // ini_line = "msg,template"

  zlen  =  GetProfileStr( Profile_TEMPLATES,key,ini_line,256 );

  tmpl_ptr  =  strchr( ini_line,',' );
  
  if ( eqn->log_level>=2 ) {  
    char buf[512];
    sprintf( buf,"\n%sTMPL : %s=!%s!\n",eqn->indent,key,ini_line );
    SetComment( strs,2,buf );
    num_strs++;
  }

  if ( tmpl_ptr )
    *tmpl_ptr++ =  0;

  if ( tmpl_ptr && *tmpl_ptr ) {
    char* ztmpl;
    strs[num_strs].log_level   =  0;
    strs[num_strs].do_delete   =  1;
    strs[num_strs].ilk         =  Z_TMPL;
    strs[num_strs].is_line     =  0;
    ztmpl =  (char*)malloc(strlen(tmpl_ptr)+1);
    strcpy( ztmpl,tmpl_ptr );
    strs[num_strs].data        =  ztmpl;
    num_strs++;
  }

  return num_strs;
}

static
void GetPileType( char* the_template,int arg_num,char* targ_nom )
{
  int di  =  0;
  char* ptr;
  char tok[4];
  sprintf( tok, "#%d", arg_num );     //#2

  ptr =  strstr( the_template,tok );
  if ( ptr && *(ptr+2)=='[' ) {
    ptr +=  3;
    while ( *ptr!=']' && di<32 )
      targ_nom[di++]  =  *ptr++;
  }

  targ_nom[di]  =  0;
}


  
// utility routines
static
void Eqn_LoadCharSetAtts(MTEquation* eqn, char** table)
{
char key[16];
char buff[16];
int slot  =  1;
int zln;

  eqn->atts_table = malloc( sizeof(CHARSETatts)*NUM_TYPEFACE_SLOTS );

  while ( slot<=NUM_TYPEFACE_SLOTS ) {
    sprintf( key, "%d", slot+128 );
    zln =  GetProfileStr( table,key,buff,16 );
    if ( zln ) {
      eqn->atts_table[slot-1].mathattr       =  buff[0] - '0';
      eqn->atts_table[slot-1].do_lookup      =  buff[2] - '0';
      eqn->atts_table[slot-1].use_codepoint  =  buff[4] - '0';
    } else {
      eqn->atts_table[slot-1].mathattr       =  1;
      eqn->atts_table[slot-1].do_lookup      =  0;
      eqn->atts_table[slot-1].use_codepoint  =  1;
    }

    slot++;
  }
}


// section contains strings of the form
//    key=data
static
int GetProfileStr( char** section, char* key, char* data, int datalen )
{
char** rover;
int keylen;

  keylen = strlen(key);
  for (rover = &section[0]; *rover; ++rover)
  {
    if (strncmp( *rover, key, keylen ) == 0) {
      strncpy( data, *rover + keylen + 1, datalen-1 );  // skip over = (no check for white space
      data[datalen-1] = 0;  // null terminate
      return( strlen(data) );
    }
  }
  return 0;
}

//data

//[FUNCTIONS]
char* Profile_FUNCTIONS[] = {
"Pr=\\Pr ",
"arccos=\\arccos ",
"arcsin=\\arcsin ",
"arctan=\\arctan ",
"arg=\\arg ",
"cos=\\cos ",
"cosh=\\cosh ",
"cot=\\cot ",
"coth=\\coth ",
"csc=\\csc ",
"deg=\\deg ",
"det=\\det ",
"dim=\\dim ",
"exp=\\exp ",
"gcd=\\gcd ",
"hom=\\hom ",
"inf=\\inf ",
"ker=\\ker ",
"lim=\\lim ",
"liminf=\\liminf ",
"limsup=\\limsup ",
"ln=\\ln ",
"log=\\log ",
"max=\\max ",
"min=\\min ",
"sec=\\sec ",
"sin=\\sin ",
"sinh=\\sinh ",
"sup=\\sup ",
"tan=\\tan ",
"tanh=\\tanh ",
"mod=\\limfunc{mod} ",
"glb=\\limfunc{glb} ",
"lub=\\limfunc{lub} ",
"int=\\limfunc{int} ",
"Im=\\limfunc{Im} ",
"Re=\\limfunc{Re} ",
"var=\\limfunc{var} ",
"cov=\\limfunc{cov} ",
0 };

//[VARIABLES]
char* Profile_VARIABLES[] = {
"STARTSUB=_{,\\Sb ",
"ENDSUB=}, \\endSb ",
"STARTSUP=^{,\\Sp ",
"ENDSUP=}, \\endSp ",
0 };

//[PILEtranslation]
char* Profile_PILEtranslation[] = {
"MATHDEFAULT=MathForce,\\begin{array}{l}, \\,\\end{array}",
"TEXTDEFAULT=TextOnly,\\begin{tabular}{l}, \\,\\end{tabular}",
"L=,, \\ ,",
"M=MathForce,\\begin{array}{l}, \\,\\end{array}",
"X=TextForce,\\begin{texttest}, \\LINE_END,\\end{texttest}",
"Y=MathForce,\\begin{mathtest}, \\LINE_END,\\end{mathtest}",
0 };


//[CHARSETatts]
//;tf#=math_att,do_lookup,use_codepoint
//;math_att 0=MA_NONE, 1 = MA_FORCE_MATH, 2 = MA_FORCE_TEXT, 3 = 2 translations
char* Profile_CHARSETatts[] = {
//;fnTEXT
"129=2,0,1",
//;fnFUNCTION
"130=1,1,1",
//;fnVARIABLE
"131=1,0,1",
//;fnLCGREEK
"132=1,1,0",
//;fnUCGREEK
"133=1,1,0",
//;fnSYMBOL
"134=1,1,1",
//;fnVECTOR
"135=1,0,1",
//;fnNUMBER
"136=1,0,1",
"137=1,1,0",
"138=1,1,0",
//;fnMTEXTRA
"139=1,1,1",
"140=1,1,0",
"141=1,1,0",
"142=1,1,0",
"143=1,1,0",
"144=1,1,0",
"145=1,1,0",
"146=1,1,0",
"147=1,1,0",
"148=1,1,0",
"149=1,1,0",
//;fnEXPAND
"150=1,1,0",
//;fnMARKER
"151=1,1,0",
//;fnSPACE
"152=3,1,0",
"153=1,1,0",
0 };

//[CHARSETatts3]
char* Profile_CHARSETatts3[] = {
//;fnTEXT
"129=2,0,1",
//;fnFUNCTION
"130=1,1,1",
//;fnVARIABLE
"131=1,0,1",
//;fnLCGREEK
"132=1,1,0",
//;fnUCGREEK
"133=1,1,0",
//;fnSYMBOL
"134=1,1,1",
//;fnVECTOR
"135=1,0,1",
//;fnNUMBER
"136=1,0,1",
"137=1,1,0",
"138=1,1,0",
//;fnMTEXTRA
"139=1,1,1",
"140=1,1,0",
"141=1,1,0",
"142=1,1,0",
"143=1,1,0",
"144=1,1,0",
"145=1,1,0",
"146=1,1,0",
"147=1,1,0",
"148=1,1,0",
"149=1,1,0",
//;fnEXPAND
"150=1,1,0",
//;fnMARKER
"151=1,1,0",
//;fnSPACE
"152=3,1,0",
"153=1,1,0",
0 };

//[CHARTABLE]
// careful with trailing spaces!
char* Profile_CHARTABLE[] = {
"130.91=\\lbrack ",
"132.74=\\vartheta ",
"132.86=\\varsigma ",
"132.97=\\alpha ",
"132.98=\\beta ",
"132.99=\\chi ",
"132.100=\\delta ",
"132.101=\\epsilon ",
"132.102=\\phi ",
"132.103=\\gamma ",
"132.104=\\eta ",
"132.105=\\iota ",
"132.106=\\varphi ",
"132.107=\\kappa ",
"132.108=\\lambda ",
"132.109=\\mu ",
"132.110=\\nu ",
"132.111=\\o ",
"132.112=\\pi ",
"132.113=\\theta ",
"132.114=\\rho ",
"132.115=\\sigma ",
"132.116=\\tau ",
"132.117=\\upsilon ",
"132.118=\\varpi ",
"132.119=\\omega ",
"132.120=\\xi ",
"132.121=\\psi ",
"132.122=\\zeta ",
"132.182=\\partial ",
"133.65=A",
"133.66=B",
"133.67=X",
"133.68=\\Delta ",
"133.69=E",
"133.70=\\Phi ",
"133.71=\\Gamma ",
"133.72=H",
"133.73=I",
"133.75=K",
"133.76=\\Lambda ",
"133.77=M",
"133.78=N",
"133.79=O",
"133.80=\\Pi ",
"133.81=\\Theta ",
"133.82=P",
"133.83=\\Sigma ",
"133.84=T",
"133.85=Y",
"133.87=\\Omega ",
"133.88=\\Xi ",
"133.89=\\Psi ",
"133.90=Z",
"134.34=\\forall ",
"134.36=\\exists ",
"134.39=\\ni ",
"134.42=*",
"134.43=+",
"134.45=-",
"134.61==",
"134.64=\\cong ",
"134.92=\therefore ",
"134.94=\\bot ",
"134.97=\\alpha ",
"134.98=\\beta ",
"134.99=\\chi ",
"134.100=\\delta ",
"134.101=\\epsilon ",
"134.102=\\phi ",
"134.103=\\gamma ",
"134.104=\\eta ",
"134.105=\\iota ",
"134.106=\\varphi ",
"134.107=\\kappa ",
"134.108=\\lambda ",
"134.109=\\mu ",
"134.110=\\nu ",
"134.112=\\pi ",
"134.113=\\theta ",
"134.114=\\rho ",
"134.115=\\sigma ",
"134.116=\\tau ",
"134.117=\\upsilon ",
"134.118=\\varpi ",
"134.119=\\omega ",
"134.120=\\xi ",
"134.121=\\psi ",
"134.122=\\zeta ",
"134.163=\\leq ",
"134.165=\\infty ",
"134.171=\\leftrightarrow ",
"134.172=\\leftarrow ",
"134.173=\\uparrow ",
"134.174=\\rightarrow ",
"134.175=\\downarrow ",
"134.176=\\degree ",
"134.177=\\pm ",
"134.179=\\geq ",
"134.180=\\times ",
"134.181=\\propto ",
"134.182=\\partial ", /* added by Ujwal S. Sathyam */
"134.183=\\bullet ",
"134.184=\\div ",
"134.185=\\neq ",
"134.186=\\equiv ",
"134.187=\\approx ",
"134.191=\\hookleftarrow ",
"134.192=\\aleph ",
"134.193=\\Im ",
"134.194=\\Re ",
"134.195=\\wp ",
"134.196=\\otimes ",
"134.197=\\oplus ",
"134.198=\\emptyset ",
"134.199=\\cap ",
"134.200=\\cup ",
"134.201=\\supset ",
"134.202=\\supseteq ",
"134.203=\\nsubset ",
"134.204=\\subset ",
"134.205=\\subseteq ",
"134.206=\\in ",
"134.207=\\notin ",
"134.208=\\angle ",
"134.209=\\nabla ",
"134.213=\\prod ",
"134.215=\\cdot ",
"134.216=\\neg ",
"134.217=\\wedge ",
"134.218=\\vee ",
"134.219=\\Leftrightarrow ",
"134.220=\\Leftarrow ",
"134.221=\\Uparrow ",
"134.222=\\Rightarrow ",
"134.223=\\Downarrow ",
"134.224=\\Diamond ",
"134.225=\\langle ",
"134.229=\\Sigma ",
"134.241=\\rangle ",
"134.242=\\smallint ",
"139.60=\\vartriangleleft ",
"139.62=\\vartriangleright ",
"139.67=\\coprod ",
"139.68=\\lambdabar ",
"139.73=\\bigcap ",
"139.75=\\ldots ",
"139.76=\\cdots ",
"139.77=\\vdots ",
"139.78=\\ddots ",
"139.79=\\ddots ",
"139.81=\\because ",
"139.85=\\bigcup ",
"139.97=\\longmapsto ",
"139.98=\\updownarrow ",
"139.99=\\Updownarrow ",
"139.102=\\succ ",
"139.104=\\hbar ",
"139.108=\\ell ",
"139.109=\\mp ",
"139.111=\\circ ",
"139.112=\\prec ",
"152.1m={}",
"152.1t={}",
"152.8m=\\/",
"152.8t=\\/",
"152.2m=\\,",
"152.2t=\\thinspace ",
"152.4m=\\;",
"152.4t=\\ ",
"152.5m=\\quad ",
"152.5t=\\quad ",
  0 };

//[CHARTABLE3]
char* Profile_CHARTABLE3[] = {
"130.91=\\lbrack ",
"132.945=\\alpha ",
"132.946=\\beta ",
"132.967=\\chi ",
"132.948=\\delta ",
"132.949=\\epsilon ",
"132.966=\\phi ",
"132.947=\\gamma ",
"132.951=\\eta ",
"132.953=\\iota ",
"132.981=\\varphi ",
"132.954=\\kappa ",
"132.955=\\lambda ",
"132.956=\\mu ",
"132.957=\\nu ",
"132.959=\\o ",
"132.960=\\pi ",
"132.952=\\theta ",
"132.961=\\rho ",
"132.963=\\sigma ",
"132.964=\\tau ",
"132.965=\\upsilon ",
"132.969=\\omega ",
"132.958=\\xi ",
"132.968=\\psi ",
"132.950=\\zeta ",
"132.977=\\vartheta ",
"132.962=\\varsigma ",
"132.982=\\varpi ",
"133.913=A",
"133.914=B",
"133.935=X",
"133.916=\\Delta ",
"133.917=E",
"133.934=\\Phi ",
"133.915=\\Gamma ",
"133.919=H",
"133.921=I",
"133.922=K",
"133.923=\\Lambda ",
"133.924=M",
"133.925=N",
"133.927=O",
"133.928=\\Pi ",
"133.920=\\Theta ",
"133.929=P",
"133.931=\\Sigma ",
"133.932=T",
"133.933=Y",
"133.937=\\Omega ",
"133.926=\\Xi ",
"133.936=\\Psi ",
"133.918=Z",
"134.42=*",
"134.43=+",
"134.8722=-",
"134.61==",
"134.8804=\\leq ",
"134.8805=\\geq ",
"134.8800=\\neq ",
"134.8801=\\equiv ",
"134.8776=\\approx ",
"134.8773=\\cong ",
"134.8733=\\propto ",
"134.177=\\pm ",
"134.215=\\times ",
"134.247=\\div ",
"134.8727=\\ast ",
"134.8901=\\cdot ",
"134.8226=\\bullet ",
"134.8855=\\otimes ",
"134.8853=\\oplus ",
"134.9001=\\langle ",
"134.9002=\\rangle ",
"134.8594=\\rightarrow ",
"134.8592=\\leftarrow ",
"134.8596=\\leftrightarrow ",
"134.8593=\\uparrow ",
"134.8595=\\downarrow ",
"134.8658=\\Rightarrow ",
"134.8656=\\Leftarrow ",
"134.8660=\\Leftrightarrow ",
"134.8657=\\Uparrow ",
"134.8659=\\Downarrow ",
"134.8629=\\hookleftarrow ",
"134.8756=\\therefore ",
"134.8717=\\backepsilon ",
"134.8707=\\exists ",
"134.8704=\\forall ",
"134.172=\\neg ",
"134.8743=\\wedge ",
"134.8744=\\vee ",
"134.8712=\\in ",
"134.8713=\\notin ",
"134.8746=\\cup ",
"134.8745=\\cap ",
"134.8834=\\subset ",
"134.8835=\\supset ",
"134.8838=\\subseteq ",
"134.8839=\\supseteq ",
"134.8836=\\not\\subset ",
"134.8709=\\emptyset ",
"134.8706=\\partial ",
"134.8711=\\nabla ",
"134.8465=\\Im ",
"134.8476=\\Re ",
"134.8501=\\aleph ",
"134.8736=\\angle ",
"134.8869=\\bot ",
"134.8900=\\lozenge ",
"134.8734=\\infty ",
"134.8472=\\wp ",
"134.176=\\degree ",
"134.8747=\\smallint",
"134.8721=\\sum ",
"134.8719=\\prod ",
"139.8230=\\ldots ",
"139.8943=\\cdots ",
"139.8942=\\vdots ",
"139.8944=\\ddots ",
"139.8945=\\ddots ",
"139.8826=\\prec ",
"139.8827=\\succ ",
"139.8882=\\vartriangleleft ",
"139.8883=\\vartriangleright ",
"139.8723=\\mp ",
"139.8728=\\circ ",
"139.8614=\\longmapsto ",
"139.8597=\\updownarrow ",
"139.8661=\\Updownarrow ",
"139.4746=\\bigcup ",
"139.4745=\\bigcap ",
"139.8757=\\because ",
"139.8467=\\ell ",
"139.8463=\\hbar ",
"139.411=\\lambdabar ",
"139.8720=\\coprod ",
"151.60160m={}",
"151.60160t={}",
"152.60161m=\\/",
"152.60161t=\\/",
"152.61168m=@,",
"152.61168t=",
"152.60162m=\\,",
"152.60162t=\\thinspace ",
"152.60164m=\\;",
"152.60164t=\\ ",
"152.60165m=\\quad ",
"152.60165t=\\quad ",
  0 };

//[TEMPLATES]
char* Profile_TEMPLATES[] = {
"0.0=fence: angle-both,\\left\\langle #1[M]\\right\\rangle ",
"0.1=fence: angle-left only,\\left\\langle #1[M]\\right. ",
"0.2=fence: angle-right only,\\left. #1[M]\\right\\rangle ",
"1.0=fence: paren-both,\\left( #1[M]\\right) ",
"1.1=fence: paren-left only,\\left( #1[M]\\right. ",
"1.2=fence: paren-right only,\\left. #1[M]\\right) ",
"2.0=fence: brace-both,\\left\\{ #1[M]\\right\\} ",
"2.1=fence: brace-left only,\\left\\{ #1[M]\\right. ",
"2.2=fence: brace-right only,\\left. #1[M]\\right\\} ",
"3.0=fence: brack-both,\\left[ #1[M]\\right] ",
"3.1=fence: brack-left only,\\lef]t[ #1[M]\\right. ",
"3.2=fence: brack-right only,\\left. #1[M]\\right] ",
"4.0=fence: bar-both,\\left| #1[M]\\right| ",
"4.1=fence: bar-left only,\\left| #1[M]\\right. ",
"4.2=fence: bar-right only,\\left. #1[M]\\right| ",
"5.0=fence: dbar-both,\\left\\| #1[M]\\right\\| ",
"5.1=fence: dbar-left only,\\left\\| #1[M]\\right. ",
"5.2=fence: dbar-right only,\\left. #1[M]\\right\\| ",
"6.0=fence: floor,\\left\\lfloor #1[M]\\right\\rfloor ",
"7.0=fence: ceiling,\\left\\lceil #1[M]\\right\\rceil ",
"8.0=fence: LBLB,\\left[ #1[M]\\right[ ",
"9.0=fence: RBRB,\\left] #1[M]\\right] ",
"10.0=fence: RBLB,\\left] #1[M]\\right[ ",
"11.0=fence: LBRP,\\left[ #1[M]\\right) ",
"12.0=fence: LPRB,\\left( #1[M]\\right] ",
"13.0=root: sqroot,\\sqrt{#1[M]} ",
"13.1=root: nthroot,\\sqrt[#2[M]]{#1[M]} ",
"14.0=fract: ffract,\\frac{#1[M]}{#2[M]} ",
"14.1=fract: pfract,\\frac{#1[M]}{#2[M]} ",
"15.0=script: super,#1[L][STARTSUP][ENDSUP] ",
"15.1=script: sub,#1[L][STARTSUB][ENDSUB] ",
"15.2=script: subsup,#1[L][STARTSUB][ENDSUB]#2[L][STARTSUP][ENDSUP] ",
"16.0=ubar: subar,\\underline{#1[M]} ",
"16.1=ubar: dubar,\\underline{\\underline{#1[M]}} ",
"17.0=obar: sobar,\\overline{#1[M]} ",
"17.1=obar: dobar,\\overline{\\overline{#1[M]}} ",
"18.0=larrow: box on top,\\stackrel{#1[M]}{\\longleftarrow} ",
"18.1=larrow: box below ,\\stackunder{#1[M]}{\\longleftarrow} ",
"19.0=rarrow: box on top,\\stackrel{#1[M]}{\\longrightarrow} ",
"19.1=rarrow: box below ,\\stackunder{#1[M]}{\\longrightarrow} ",
"20.0=barrow: box on top,\\stackrel{#1[M]}{\\longleftrightarrow} ",
"20.1=barrow: box below ,\\stackunder{#1[M]}{\\longleftrightarrow} ",
"21.0=integrals: single - no limits,\\int #1[M] ",
"21.1=integrals: single - lower only,\\int\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"21.2=integrals: single - both,\\int\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"21.3=integrals: contour - no limits,\\oint #1[M] ",
"21.4=integrals: contour - lower only,\\oint\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"22.0=integrals: double - no limits ,\\iint #1[M] ",
"22.1=integrals: double - lower only,\\iint\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"22.2=integrals: area - no limits ,\\iint #1[M] ",
"22.3=integrals: area - lower only,\\iint\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"23.0=integrals: triple - no limits ,\\iiint #1[M] ",
"23.1=integrals: triple - lower only,\\iiint\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"23.2=integrals: volume - no limits ,\\iiint #1[M] ",
"23.3=integrals: volume - lower only,\\iiint\\nolimits#2[L][STARTSUB][ENDSUB] #1[M] ",
"24.0=integrals: single - sum style - both,\\int\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"24.1=integrals: single - sum style - lower only,\\int\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"24.2=integrals: contour - sum style - lower only,\\oint\\limits#2[L][STARTSUB][ENDSUB] #1[M] ",
"25.0=integrals: area - sum style - lower only,\\iint\\limits#2[L][STARTSUB][ENDSUB] #1[M] ",
"25.1=integrals: double - sum style - lower only,\\iint\\limits#2[L][STARTSUB][ENDSUB] #1[M] ",
"26.0=integrals: volume - sum style - lower only,\\iiint\\limits#2[L][STARTSUB][ENDSUB] #1[M] ",
"26.1=integrals: triple - sum style - lower only,\\iiint\\limits#2[L][STARTSUB][ENDSUB] #1[M] ",
"27.0=horizontal braces: upper,\\stackrel{#2[M]}{\\overbrace{#1[M]}} ",
"28.0=horizontal braces: lower,\\stackunder{#2[M]}{\\underbrace{#1[M]}} ",
"29.0=sum: limits top/bottom - lower only,\\sum\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"29.1=sum: limits top/bottom - both,\\sum\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"29.2=sum: no limits,\\sum #1[M] ",
"30.0=sum: limits right - lower only,\\sum\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"30.1=sum: limits right - both,\\sum\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"31.0=product: limits top/bottom - lower only,\\dprod\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"31.1=product: limits top/bottom - both,\\dprod\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"31.2=product: no limits,\\dprod #1[M] ",
"32.0=product: limits right - lower only,\\dprod\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"32.1=product: limits right - both,\\dprod\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"33.0=coproduct: limits top/bottom - lower only,\\dcoprod\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"33.1=coproduct: limits top/bottom - both,\\dcoprod\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"33.2=coproduct: no limits,\\dcoprod #1[M] ",
"34.0=coproduct: limits right - lower only,\\dcoprod\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"34.1=coproduct: limits right - both,\\dcoprod\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"35.0=union: limits top/bottom - lower only,\\dbigcup\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"35.1=union: limits top/bottom - both,\\dbigcup\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"35.2=union: no limits,\\dbigcup #1[M] ",
"36.0=union: limits right - lower only,\\dbigcup\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"36.1=union: limits right - both,\\dbigcup\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"37.0=intersection: limits top/bottom - lower only,\\dbigcap\\limits#2[L][STARTSUB][ENDSUB]#1[M] ",
"37.1=intersection: limits top/bottom - both,\\dbigcap\\limits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"37.2=intersection: no limits,\\dbigcap #1[M] ",
"38.0=intersection: limits right - lower only,\\dbigcap\\nolimits#2[L][STARTSUB][ENDSUB]#1[M] ",
"38.1=intersection: limits right - both,\\dbigcap\\nolimits#2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP]#1[M] ",
"39.0=limit: upper,#1 #2[L][STARTSUP][ENDSUP] ",
"39.1=limit: lower,#1 #2[L][STARTSUB][ENDSUB] ",
"39.2=limit: both,#1 #2[L][STARTSUB][ENDSUB]#3[L][STARTSUP][ENDSUP] ",
"40.0=long divisionW,",
"40.1=long divisionWO,",
"41.0=slash fraction: normal,\\frac{#1[M]}{#2[M]} ",
"41.1=slash fraction: baseline,#1[M]/#2[M] ",
"41.2=slash fraction: subscript-sized,\\frac{#1[M]}{#2[M]} ",
"42.0=INTOP: upper,",
"42.1=INTOP: lower,",
"42.2=INTOP: both,",
"43.0=SUMOP: upper,",
"43.1=SUMOP: lower,",
"43.2=SUMOP: both,",
"44.0=leadingSUPER,",
"44.1=leadingSUB,",
"44.2=leadingSUBSUP,",
"45.0=Dirac: both,\\left\\langle #1[M]\\right.\\left| #2[M]\\right\\rangle ",
"45.1=Dirac: left,\\left\\langle #1[M]\\right| ",
"45.2=Dirac: right,\\left| #1[M]\\right\\rangle ",
"46.0=under arrow: left,\\underleftarrow{#1[M]} ",
"46.1=under arrow: right,\\underrightarrow{#1[M]} ",
"46.2=under arrow: both,\\underleftrightarrow{#1[M]} ",
"47.0=over arrow: left,\\overleftarrow{#1[M]} ",
"47.1=over arrow: right,\\overrightarrow{#1[M]} ",
"47.2=over arrow: both,\\overleftrightarrow{#1[M]} ",
"TextInMath=\text{#1} ",
0 };


//[EMBELLS]
//;format is "math template,text template" (different from all the above)
char* Template_EMBELLS[] = {
/* embDOT     */ "\\dot{%1} ,\\.%1 ",
/* embDDOT    */ "\\ddot{%1} ,\\\"%1 ",
/* embTDOT    */ "\\dddot{%1} ,%1 ",
/* embPRIME   */ "%1\\prime ,%1 ",
/* embDPRIME  */ "%1\\prime \\prime ,%1 ",
/* embBPRIME  */ "%1' , %1",
/* embTILDE   */ "\\tilde{%1} ,\\~%1 ",
/* embHAT     */ "\\hat{%1} ,\\^%1 ",
/* embNOT     */ "\\not %1 ,\\NEG %1 ",
/* embRARROW  */ "\\vec{%1} ,%1 ",
/* embLARROW  */ "\\overleftarrow1{%1} ,%1 ",
/* embBARROW  */ "\\vec{%1} ,%1 ",
/* embR1ARROW */ "\\overleftarrow{%1} ,%1 ",
/* embL1ARROW */ "\\overleftrightarrow{%1} ,%1 ",
/* embMBAR    */ "\\underline{%1} ,%1 ",
/* embOBAR    */ "\\bar{%1} ,\\=%1 ",
/* embTPRIME  */ "%1\\prime \\prime \\prime ,",
/* embFROWN   */ "\\widehat{%1} ,%1 ",
/* embSMILE   */ "\\breve{%1} ,%1 ",
0 };

