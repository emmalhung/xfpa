C**********************************************************************C
C                                                                      C
C     Version 8 (c) Copyright 2011 Environment Canada                  C
C                                                                      C
C   This file is part of the Forecast Production Assistant (FPA).      C
C   The FPA is free software: you can redistribute it and/or modify it C
C   under the terms of the GNU General Public License as published by  C
C   the Free Software Foundation, either version 3 of the License, or  C
C   any later version.                                                 C
C                                                                      C
C   The FPA is distributed in the hope that it will be useful, but     C
C   WITHOUT ANY WARRANTY; without even the implied warranty of         C
C   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               C
C   See the GNU General Public License for more details.               C
C                                                                      C
C   You should have received a copy of the GNU General Public License  C
C   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   C
C                                                                      *
C**********************************************************************C
C**********************************************************************C
C                                                                      C
C  SUBROUTINE FTN_MODEL_SETUP(ALIASFILE, FTNOK)                        C
C                                                                      C
C  This subroutine is used to read the file which contains the alias   C
C  and full path name for each file required by the Model.             C
C                                                                      C
C  Input:  ALIASFILE  Full path name of file containing Model aliases  C
C  Output: FTNOK      Integer return (return 0 for OK)                 C
C***********************************************************************
C
      SUBROUTINE FTN_MODEL_SETUP(ALIASFILE, FTNOK)
C
      CHARACTER*(*) ALIASFILE
      INTEGER       FTNOK
C
      INTEGER       NREC, NSTAT
      CHARACTER*32  ALIAS
      CHARACTER*132 PATH
C
      INTEGER       NUMFILES, MAXALIAS
      DATA MAXALIAS /25/
      CHARACTER*32  FILEALIAS
      CHARACTER*132 FILEPATH
      COMMON /FTN_MODEL_FILE_ALIAS_LIST/
     $         NUMFILES, FILEALIAS(25), FILEPATH(25)
C
C  INITIALIZE NUMBER OF ALIASES IN ALIAS FILE
C
      DATA NUMFILES /0/
C
C  OPEN FILE CONTAINING ALIASED FILE NAMES FOR MODEL
C
      CLOSE (UNIT=4)
      OPEN (UNIT=4, STATUS='OLD', FILE=ALIASFILE, ERR=910, IOSTAT=NSTAT)
C
C  READ CONTENTS OF FILE CONTAINING ALIASED FILE NAMES FOR MODEL
C
      NREC = 0
      DO WHILE (.TRUE.)
        READ(4, *, END=100, ERR=920, IOSTAT=NSTAT) ALIAS, PATH
        NREC = NREC + 1
        IF (NREC .GT. MAXALIAS) GOTO 930
C
C  SET ALIAS AND FULL PATH NAME INTO COMMON
C
        FILEALIAS(NREC) = ALIAS
        FILEPATH(NREC)  = PATH
      ENDDO
C
C  CHECK THAT SOME DATA HAS BEEN READ WHEN END OF FILE REACHED
C
 100  IF (NREC .LE. 0) GO TO 940
C
C  RETURN FOR SUCCESSFUL COMPLETION
C
      CLOSE (UNIT=4)
      NUMFILES = NREC
      FTNOK = 0
      RETURN
C
C  ERROR RETURN FOR PROBLEMS OPENING ALIAS FILE
C
 910  WRITE(6, 915) ALIASFILE, NSTAT
 915  FORMAT(/' ERROR OPENING ALIAS FILE - ', A132,
     &       /'  NSTAT = ', I4)
      FTNOK = -1
      RETURN
C
C  ERROR RETURN FOR PROBLEMS READING ALIAS FILE
C
 920  WRITE(6, 925) ALIASFILE, NSTAT
 925  FORMAT(/' ERROR READING ALIAS FILE - ', A132,
     &       /'  NSTAT = ', I4)
      FTNOK = -2
      RETURN
C
C  ERROR RETURN FOR TOO MANY FILES IN ALIAS FILE
C
 930  WRITE(6, 935) ALIASFILE, MAXALIAS
 935  FORMAT(/' TOO MANY ALIASES IN ALIAS FILE - ', A132,
     &       /'  MAXIMUM NUMBER OF ALIASES = ', I4)
      FTNOK = -3
      RETURN
C
C  ERROR RETURN FOR NO ALIASES IN ALIAS FILE
C
 940  WRITE(6, 945) ALIASFILE
 945  FORMAT(/' NO ALIASES IN ALIAS FILE - ', A132)
      FTNOK = -4
      RETURN
      END
C
C**********************************************************************C
C                                                                      C
C  SUBROUTINE FTN_MODEL_FILE_OPEN(LU, FSTAT, FFORM, FNAME, FTNOK)      C
C                                                                      C
C  This subroutine is used to match the file name identifier string    C
C  to the Model aliases, and to open the corresponding file given by   C
C  the full path name for that alias.                                  C
C                                                                      C
C  Input:  LU         Logical Unit number                              C
C          FSTAT      File status identifier string                    C
C          FFORM      File format identifier string                    C
C          FNAME      File name identifier string                      C
C  Output: FTNOK      Integer return (return 0 for OK)                 C
C***********************************************************************
C
      SUBROUTINE FTN_MODEL_FILE_OPEN(LU, FSTAT, FFORM, FNAME, FTNOK)
C
      INTEGER       LU, FTNOK
      CHARACTER*(*) FSTAT, FFORM, FNAME
C
      INTEGER       NFILE, NSTAT
C
      INTEGER       NUMFILES, MAXALIAS
      DATA MAXALIAS /25/
      CHARACTER*32  FILEALIAS
      CHARACTER*132 FILEPATH
      COMMON /FTN_MODEL_FILE_ALIAS_LIST/
     $         NUMFILES, FILEALIAS(25), FILEPATH(25)
C
C  SEARCH ALIAS LIST FOR FULL PATH NAME OF FILE
C
      IF (NUMFILES .LE. 0) GO TO 910
      DO NFILE=1, NUMFILES
        IF (FNAME .EQ. FILEALIAS(NFILE)) GO TO 100
      ENDDO
C
C  ERROR IF ALIAS CANNOT BE FOUND
C
      GO TO 910
C
C  OPEN FILE USING FULL PATHNAME
C
 100  OPEN(UNIT=LU, STATUS=FSTAT, FORM=FFORM, FILE=FILEPATH(NFILE),
     & ERR=920, IOSTAT=NSTAT)
C
C  RETURN FOR SUCCESSFUL COMPLETION
C
      FTNOK = 0
      RETURN
C
C  ERROR RETURN FOR PROBLEMS MATCHING ALIAS
C
 910  WRITE(6, 915) FNAME
 915  FORMAT(/' ERROR MATCHING FILE ALIAS - ', A32)
      FTNOK = -1
      RETURN
C
C  ERROR RETURN FOR PROBLEMS OPENING FILE
C
 920  WRITE(6, 925) FILEPATH(NFILE), NSTAT
 925  FORMAT(/' ERROR OPENING FILE - ', A132,
     &       /'  NSTAT = ', I4)
      FTNOK = -2
      RETURN
      END
C
C**********************************************************************C
C                                                                      C
C  SUBROUTINE FTN_TO_C_STRING(FILENAME, FTNOK)                         C
C                                                                      C
C  This subroutine is used to convert a Fortran character string to C  C
C                                                                      C
C  Input:  FSTRING  Fortran character string                           C
C  Output: CSTRING  C character string                                 C
C***********************************************************************
C
      SUBROUTINE FTN_TO_C_STRING(FSTRING, CSTRING)
C
      CHARACTER*(*) FSTRING, CSTRING
      INTEGER       IFLEN, ICLEN, ICOUNT
C
C  RETURN IMMEDIATELY FOR EMPTY FORTRAN STRINGS
C
      IF (FSTRING .EQ. ' ') THEN
        CSTRING(1:) = ''//CHAR(0)
      ENDIF
C
C  DETERMINE LENGTH OF NON-BLANK FORTRAN STRING (COUNTING FROM END)
C
      IFLEN  = LEN(FSTRING)
      ICOUNT = IFLEN
      DO WHILE (FSTRING(ICOUNT:ICOUNT).EQ.' ')
        ICOUNT = ICOUNT - 1
      ENDDO
C
C  RETURN IMMEDIATELY IF FORTRAN STRING WILL BE
C  GREATER THAN MAXIMUM ALLOWED LENGTH
C
      ICLEN  = LEN(CSTRING)
      IF (ICOUNT .GE. ICLEN) THEN
        CSTRING(1:) = ''//CHAR(0)
      ENDIF
C
C  ADD NULL TERMINATOR TO END OF FORTRAN CHARACTER STRING
C
      CSTRING = FSTRING
      CSTRING(ICOUNT+1:) = ''//CHAR(0)
C
      RETURN
      END
C
C**********************************************************************C
C                                                                      C
C  SUBROUTINE C_TO_FTN_STRING(FILENAME, FTNOK)                         C
C                                                                      C
C  This subroutine is used to convert a C character string to Fortran  C
C                                                                      C
C  Input:  CSTRING  C character string                                 C
C  Output: FSTRING  Fortran character string                           C
C***********************************************************************
C
      SUBROUTINE C_TO_FTN_STRING(CSTRING, FSTRING)
C
      CHARACTER*(*) CSTRING, FSTRING
      INTEGER       IFLEN, ICOUNT
C
C  FIND LOCATION OF NULL TERMINATOR
C
      ICOUNT = INDEX(CSTRING, CHAR(0))
C
C  RETURN INPUT STRING IF NULL TERMINATOR NOT FOUND
C
      IF (ICOUNT .LT. 1) THEN
        FSTRING = CSTRING
C
C  SET NULL TERMINATOR TO BLANK
C
      ELSE
        FSTRING = CSTRING
        IFLEN  = LEN(FSTRING)
        IF (ICOUNT .LE. IFLEN) FSTRING(ICOUNT:) = ''
      ENDIF
C
      RETURN
      END
