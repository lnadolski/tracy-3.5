/* Tracy-2

   J. Bengtsson, CBP, LBL      1990 - 1994   Pascal version
                 SLS, PSI      1995 - 1997
   M. Boege      SLS, PSI      1998          C translation
   L. Nadolski   SOLEIL        2002          Link to NAFF, Radia field maps
   J. Bengtsson  NSLS-II, BNL  2004 -        

   t2ring.c -- Routines for closed beam lines

*/


#define n 4
void LatticeType::GetNu(Vector2 &nu, Matrix &M)
{
  int    i;
  double sgn, detp, detm, b, c, th, tv, b2mc, x_arg, y_arg;
  Matrix M1;

  Lattice.param.stable = true;

  CopyMat((long)n, M, M1);

  for (i = 0; i < n; i++)
    M1[i][i] -= 1.0;   
  detp = DetMat((long)n, M1); /* det(M-I) */

  for (i = 0; i < n; i++)
    M1[i][i] += 2.0;
  detm = DetMat((long)n, M1); /* det(M+I) */

  for (i = 0; i < n; i++)
    M1[i][i] -= 1.0; /* restore M */

  b = (detp-detm)/16.0; c = (detp+detm)/8.0 - 1.0;
  th = (M1[0][0]+M1[1][1])/2.0; tv = (M1[2][2]+M1[3][3])/2.0;

  b2mc = b*b - c;

  if (b2mc < 0.0) {
    Lattice.param.stable = false; nu[0] = -1.0; nu[1] = -1.0;
    printf("GetNu: unstable\n");
    return;
  }

  sgn = (th > tv)? 1.0 : -1.0;

  x_arg = -b + sgn*sqrt(b2mc);
  if (fabs(x_arg) <= 1.0) {
    nu[X_] = acos(x_arg)/(2.0*M_PI);
    if (M1[0][1] < 0.0) nu[X_] = 1.0 - nu[X_];
  } else {
    Lattice.param.stable = false; nu[X_] = -1.0;
    printf("GetNu: unstable (horizontal)\n");
  }

  y_arg = -b - sgn*sqrt(b2mc);
  if (fabs(y_arg) <= 1.0) {
    nu[Y_] = acos(y_arg)/(2.0*M_PI);
    if (M1[2][3] < 0.0) nu[Y_] = 1.0 - nu[Y_];
  } else {
    Lattice.param.stable = false; nu[Y_] = -1.0;
    printf("GetNu: unstable (vertical)\n");
    return;
  }

  return;
}
#undef n


void LatticeType::Cell_GetABGN(Matrix &M, Vector2 &alpha, Vector2 &beta,
				Vector2 &gamma, Vector2 &nu)
{
  int    i = 0, j = 0;
  double c = 0.0, s = 0.0;

  Lattice.param.stable = true;
  for (i = 0; i <= 1; i++) {
    j = (i+1)*2 - 1;
    /* c=cos(mu) = Tr(N)/2 */
    c = (M[j-1][j-1]+M[j][j])/2.0;
    Lattice.param.stable = (fabs(c) < 1.0);
    if (Lattice.param.stable) {
      // s = sin(mu)
      s = sqrt(1.0-c*c)*sgn(M[j-1][j]);
      alpha[i] = (M[j-1][j-1]-M[j][j])/(2.0*s); beta[i]  =  M[j-1][j]/s;
      gamma[i] = -(M[j][j-1]/s);
//      nu[i] = acos(c)/2/pi;
      GetNu(nu, M);
    }
  }
}


#define n 4
void LatticeType::Cell_Geteta(long i0, long i1, bool ring, double dP)
{
  long int i = 0, lastpos = 0;
  int      j = 0, k = 0;
  psVector xref;
  psVector codbuf[Cell_nLocMax+1];
  CellType *cellp;

  /* cod for the energy dP - Lattice.param.dPcommon / 2e0 */
  if (ring)
    Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps,
	   dP-Lattice.param.dPcommon/2e0, lastpos);
  else { /* beam mode */
    CopyVec(n+2, Lattice.param.CODvect, xref);
    xref[4] = dP - Lattice.param.dPcommon/2e0;
    Cell_Pass(i0, i1, xref, lastpos);
  }

  /* Store chromatic orbit for elements i0 to i1 in codbuf */
  for (i = i0; i <= i1; i++)
    CopyVec(n+2, Lattice.Cell[i]->BeamPos, codbuf[i]);

  /* cod for the energy dP - Lattice.param.dPcommon / 2e0 */
  if (ring)
    Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps,
		   dP+Lattice.param.dPcommon/2e0, lastpos);
  else { /* beam mode */
    CopyVec(n+2, Lattice.param.CODvect, xref);
    xref[4] = dP + Lattice.param.dPcommon/2e0;
    Cell_Pass(i0, i1, xref, lastpos);
  }

  for (i = i0; i <= i1; i++) {
    cellp = Lattice.Cell[i];
    for (j = 1; j <= 2; j++) {
      k = j*2 - 1;
      cellp->Eta[j-1] = (cellp->BeamPos[k-1]-codbuf[i][k-1])
	/Lattice.param.dPcommon;
      cellp->Etap[j-1] = (cellp->BeamPos[k]-codbuf[i][k])
	/Lattice.param.dPcommon;
    }
  }
}
#undef n


#define n 4
void getprm(Matrix &Ascr, Vector2 &alpha, Vector2 &beta)
{
  int i, j;

  for (i = 1; i <= 2; i++) {
    j = i*2 - 1;
    alpha[i-1] = -(Ascr[j-1][j-1]*Ascr[j][j-1] + Ascr[j-1][j]*Ascr[j][j]);
    beta[i-1] = sqr(Ascr[j-1][j-1]) + sqr(Ascr[j-1][j]);
  }
}
#undef n


#define n 4

void dagetprm(ss_vect<tps> &Ascr, Vector2 &alpha, Vector2 &beta)
{
  int i = 0, j = 0;

  for (i = 1; i <= 2; i++) {
    j = i*2 - 1;
    alpha[i-1] = -(getmat(Ascr, j, j)*getmat(Ascr, j+1, j)
                 + getmat(Ascr, j, j+1)*getmat(Ascr, j+1, j+1));
    beta[i-1] = sqr(getmat(Ascr, j, j)) + sqr(getmat(Ascr, j, j+1));
  }
}


void LatticeType::Cell_Twiss(long i0, long i1, ss_vect<tps> &Ascr, bool chroma,
			      bool ring, double dP)
{
  long int     i = 0;
  int          j = 0, k = 0;
  Vector2      nu1, dnu;   /* absolute and relative phase advance */
  ss_vect<tps> Ascr0, Ascr1;
  CellType     *cellp;

  /* initialization */
  for (j = 0; j <= 1; j++) {
    nu1[j] = 0.0; dnu[j] = 0.0; 
  }

  if (Lattice.param.radiation) Lattice.param.dE = 0.0;

  cellp = Lattice.Cell[i0];
  dagetprm(Ascr, cellp->Alpha, cellp->Beta);
  memcpy(cellp->Nu, nu1, sizeof(Vector2));

  Ascr0 = Ascr;
  for (j = 0; j <= n+1; j++)
    Ascr0[j] += tps(Lattice.param.CODvect[j]);

  Ascr1 = Ascr0;
  for (i = i0; i <= i1; i++) {
    Lattice.Cell[i]->Propagate(Ascr1);
    cellp = Lattice.Cell[i];
    dagetprm(Ascr1, cellp->Alpha, cellp->Beta);
    for (j = 0; j <= 1; j++) {
      k = (j+1)*2 - 1;
      dnu[j] =
	(GetAngle(getmat(Ascr1, k, k), getmat(Ascr1, k, k+1)) -
	 GetAngle(getmat(Ascr0, k, k), getmat(Ascr0, k, k+1)))/(2.0*M_PI);

      if ((cellp->L >= 0.0) && (dnu[j] < -1e-16))
	dnu[j] += 1.0;
      else if ((cellp->L < 0.0) && (dnu[j] > 1e-16))
	dnu[j] -= 1.0;

      nu1[j] += dnu[j];

      cellp->Nu[j] = nu1[j];
      // cellp->Eta[j] = getmat(Ascr1, k, 5)*getmat(Ascr1, 6, 6) -
      //                getmat(Ascr1, k, 6)*getmat(Ascr1, 6, 5);
      // cellp->Etap[j] = getmat(Ascr1, k+1, 5)*getmat(Ascr1, 6, 6) -
      //                 getmat(Ascr1, k+1, 6)*getmat(Ascr1, 6, 5);
      cellp->Eta[j] = getmat(Ascr1, k, 5);
      cellp->Etap[j] = getmat(Ascr1, k+1, 5);
    }
    Ascr0 = Ascr1;
  }

  if (chroma && !Lattice.param.Cavity_on) Cell_Geteta(i0, i1, ring, dP);
}

#undef n


#define n 4
void LatticeType::Ring_Getchrom(double dP)
{
  long int lastpos = 0;
  int      j;
  Vector2  alpha = {0.0, 0.0}, beta = {0.0, 0.0}, gamma = {0.0, 0.0};
  Vector2  nu = {0.0, 0.0}, nu0 = {0.0, 0.0};

  if (dP != 0.0)
    fprintf(stdout,"Ring_Getchrom: Warning this is NOT the CHROMA, dP=%e\n",
	    dP);
  
  /* Get cod for energy dP - Lattice.param.dPcommon*/
  Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps,
		 dP-Lattice.param.dPcommon*0.5, lastpos);
  
  if (!status.codflag) {
    /* if no cod */
    fprintf(stdout,"Ring_Getchrom:  Lattice is unstable for"
	    " dP-Lattice.param.dPcommon=% .5e\n",
	    dP-Lattice.param.dPcommon*0.5);
    return;
  }
  
  /* get tunes for energy dP - Lattice.param.dPcommon/2 from oneturn matrix */
  Cell_GetABGN(Lattice.param.OneTurnMat, alpha, beta, gamma, nu0);
  
  /* Get cod for energy dP+Lattice.param.dPcommon*/
  Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps,
		 dP+Lattice.param.dPcommon*0.5, lastpos);
  
  if (!status.codflag) { /* if no cod */
    fprintf(stdout,"Ring_Getchrom  Lattice is unstable for"
	    " dP+Lattice.param.dPcommon=% .5e \n",
	    dP+Lattice.param.dPcommon*0.5);
    return;
  }

  /* get tunes for energy dP+Lattice.param.dPcommon/2 from oneturn matrix */
  Cell_GetABGN(Lattice.param.OneTurnMat, alpha, beta, gamma, nu);
  
  if (!Lattice.param.stable) {
    printf("Ring_Getchrom:  Lattice is unstable\n");
  }

  /* Get chromaticities by numerical differentiation*/
  for (j = 0; j <= 1; j++)
    Lattice.param.Chrom[j] = (nu[j]-nu0[j])/Lattice.param.dPcommon;
  
  status.chromflag = true;
}
#undef n


void LatticeType::Ring_Twiss(bool chroma, double dP)
{
  long int     lastpos = 0;
  int          n = 0;
  Vector2      alpha={0.0, 0.0}, beta={0.0, 0.0};
  Vector2      gamma={0.0, 0.0}, nu={0.0, 0.0};
  Matrix       R;
  ss_vect<tps> AScr;

  n = (Lattice.param.Cavity_on)? 6 : 4;

  Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP, lastpos);

  if (!status.codflag) return;

  // Check if stable
  Cell_GetABGN(Lattice.param.OneTurnMat, alpha, beta, gamma, nu);
  // Get eigenvalues and eigenvectors for the one turn transfer matrix
  GDiag(n, Lattice.Cell[Lattice.param.Cell_nLoc]->S, Lattice.param.Ascr,
	Lattice.param.Ascrinv, R, Lattice.param.OneTurnMat, Lattice.param.Omega,
	Lattice.param.Alphac);

  // putlinmat(n, Lattice.param.Ascr, AScr);
  putlinmat(6, Lattice.param.Ascr, AScr);
  if (!Lattice.param.Cavity_on) {
    // AScr[delta_] = 0.0; AScr[ct_] = 0.0;
    AScr[delta_] = tps(0e0, delta_+1); AScr[ct_] = 0e0;
  }

  Cell_Twiss(0, Lattice.param.Cell_nLoc, AScr, chroma, true, dP);

  memcpy(Lattice.param.TotalTune, Lattice.Cell[Lattice.param.Cell_nLoc]->Nu,
	 sizeof(Vector2));
  status.tuneflag = true;

  if (chroma && !Lattice.param.Cavity_on) {
    Ring_Getchrom(dP);
    Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP, lastpos);
  }
}


void LatticeType::Ring_GetTwiss(bool chroma, double dP)
{

  if (trace) printf("enter ring_gettwiss\n");
  Ring_Twiss(chroma, dP);
  Lattice.param.Alphac =
    Lattice.param.OneTurnMat[ct_][delta_]
    /Lattice.Cell[Lattice.param.Cell_nLoc]->S;
  if (trace) printf("exit ring_gettwiss\n");
}


/* Local variables for Ring_Fittune: */
struct LOC_Ring_Fittune
{
  jmp_buf _JL999;
};


#define dP 0.0
void shiftk(long Elnum, double dk, struct LOC_Ring_Fittune *LINK)
{
  CellType  *cellp;
  MpoleType *M;

  cellp = Lattice.Cell[Elnum];
  M = static_cast<MpoleType*>(cellp);
  M->Bpar[Quad+HOMmax] += dk;
  Mpole_SetB(cellp->Fnum, cellp->Knum, (long)Quad);
}


void checkifstable(struct LOC_Ring_Fittune *LINK)
{
  if (!Lattice.param.stable) {
    printf("  lattice is unstable\n");
    longjmp(LINK->_JL999, 1);
  }
}


void LatticeType::Ring_Fittune(Vector2 &nu, double eps, iVector2 &nq,
				long qf[], long qd[], double dkL, long imax)
{
  struct LOC_Ring_Fittune V;

  int      i, j, k;
  Vector2  nu0, nu1;
  psVector dkL1, dnu;
  Matrix   A;

  if (setjmp(V._JL999)) return;

  if (trace)
    printf("  Tune fit, nux =%10.5f, nuy =%10.5f, eps =% .3E,"
	   " imax =%4ld, dkL = % .5E\n", nu[0], nu[1], eps, imax, dkL);
  Ring_GetTwiss(false, dP); checkifstable(&V);
  memcpy(nu0, Lattice.param.TotalTune, sizeof(Vector2));
  i = 0;
  do {
    i++;
    /* First vary kf then kd */
    for (j = 1; j <= 2L; j++) {
      for (k = 0; k < nq[j-1]; k++) {
        if (j == 1)
          shiftk(qf[k], dkL, &V); // new value for qf
        else
          shiftk(qd[k], dkL, &V); // new value for qd
      }
      Ring_GetTwiss(false, dP);
      nu1[0] = Lattice.param.TotalTune[0]; nu1[1] = Lattice.param.TotalTune[1];
      // Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP,
      // 		     lastpos);
      // Cell_GetABGN(Lattice.param.OneTurnMat, alpha, beta, gamma, nu1);
      checkifstable(&V);
      for (k = 0; k <= 1; k++) {
        dnu[k] = nu1[k] - (long)nu1[k] - nu0[k] + (long)nu0[k];
        if (fabs(dnu[k]) > 0.5) dnu[k] = 1 - fabs(dnu[k]);
        A[k][j-1] = dnu[k]/dkL;
      }

      /* Restore strength */
      for (k = 0; k < nq[j-1]; k++) {
        if (j == 1)
          shiftk(qf[k], -dkL, &V);
        else
          shiftk(qd[k], -dkL, &V);
        }
      }

    if (!InvMat(2L, A)) {
      printf("  A is singular\n");
      return;
    }

    for (j = 0; j <= 1; j++)
      dkL1[j] = nu[j] - nu0[j];
    LinTrans(2L, A, dkL1);
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < nq[j-1]; k++) {
	if (j == 1)
	  shiftk(qf[k], dkL1[j-1], &V);
	else
	  shiftk(qd[k], dkL1[j-1], &V);
      }
    }
    Ring_GetTwiss(false, dP); checkifstable(&V);
    memcpy(nu0, Lattice.param.TotalTune, sizeof(Vector2));
    if (trace)
      printf("  Nux = %10.6f%10.6f, Nuy = %10.6f%10.6f,"
	     " QF*L = % .5E, QD*L = % .5E @%3d\n",
	     nu0[0], nu1[0], nu0[1], nu1[1],
	     Lattice.Elem_GetKval(Lattice.Cell[qf[0]]->Fnum, 1, (long)Quad),
	     Lattice.Elem_GetKval(Lattice.Cell[qd[0]]->Fnum, 1, (long)Quad), i);
  } while (sqrt(sqr(nu[0]-nu0[0])+sqr(nu[1]-nu0[1])) >= eps && i != imax);
}
#undef dP


#define dP 0.0
void shiftkp(long Elnum, double dkp)
{
  CellType  *cellp;
  MpoleType *M;

  cellp = Lattice.Cell[Elnum];
  M = static_cast<MpoleType*>(cellp);
  M->Bpar[Sext+HOMmax] += dkp;
  Mpole_SetB(cellp->Fnum, cellp->Knum, (long)Sext);
}


void LatticeType::Ring_Fitchrom(Vector2 &ksi, double eps, iVector2 &ns,
				 long sf[], long sd[], double dkpL, long imax)
{
  bool     rad;
  long int lastpos;
  int      i, j, k;
  Vector2  ksi0;
  psVector dkpL1, dksi;
  Matrix   A;

  if (trace)
    printf("  Chromaticity fit, ksix =%10.5f, ksiy =%10.5f, eps =% .3E"
	   ", imax =%4ld, dkpL =%10.5f\n", ksi[0], ksi[1], eps, imax, dkpL);

  /* Turn off radiation */
  rad = Lattice.param.radiation; Lattice.param.radiation = false;
  Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP, lastpos);
  Ring_Getchrom(dP);
  for (j = 0; j <= 1; j++)
    ksi0[j] = Lattice.param.Chrom[j];
  i = 0;
  do {
    i++;
    /* First vary sf then sd */
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], dkpL);
	else
	  shiftkp(sd[k], dkpL);
      }
      Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP, lastpos);
      Ring_Getchrom(dP);
      for (k = 0; k <= 1; k++) {
	dksi[k] = Lattice.param.Chrom[k] - ksi0[k];
	A[k][j-1] = dksi[k] / dkpL;
      }
      /* Restore strength */
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], -dkpL);
	else
	  shiftkp(sd[k], -dkpL);
      }
    }
    if (!InvMat(2L, A)) {
      printf("  A is singular\n");
      goto _L999;
    }
    for (j = 0; j <= 1; j++)
      dkpL1[j] = ksi[j] - ksi0[j];
    LinTrans(2L, A, dkpL1);
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], dkpL1[j-1]);
	else
	  shiftkp(sd[k], dkpL1[j-1]);
      }
    }
    Lattice.GetCOD(Lattice.param.CODimax, Lattice.param.CODeps, dP, lastpos);
    Ring_Getchrom(dP);
    for (j = 0; j <= 1; j++)
      ksi0[j] = Lattice.param.Chrom[j];
    if (trace)
      printf("  ksix =%10.6f, ksiy =%10.6f, SF = % .5E, SD = % .5E @%3d\n",
	     ksi0[0], ksi0[1],
	     Elem_GetKval(Lattice.Cell[sf[0]]->Fnum, 1, (long)Sext),
	     Elem_GetKval(Lattice.Cell[sd[0]]->Fnum, 1, (long)Sext), i);
  } while (sqrt(sqr(ksi[0]-ksi0[0])+sqr(ksi[1]-ksi0[1])) >= eps && i != imax);
_L999:
  /* Restore radiation */
  Lattice.param.radiation = rad;
}

#undef dP


#define dP 0.0

/* Local variables for Ring_FitDisp: */
struct LOC_Ring_FitDisp
{
  jmp_buf _JL999;
};


static void shiftk_(long Elnum, double dk, struct LOC_Ring_FitDisp *LINK)
{
  CellType  *cellp;
  MpoleType *M;

  cellp = Lattice.Cell[Elnum];
  M = static_cast<MpoleType*>(cellp);
  M->Bpar[Quad+HOMmax] += dk;
  Mpole_SetB(cellp->Fnum, cellp->Knum, (long)Quad);
}


void checkifstable_(struct LOC_Ring_FitDisp *LINK)
{
  if (!Lattice.param.stable) {
    printf("  lattice is unstable\n");
    longjmp(LINK->_JL999, 1);
  }
}


void LatticeType::Ring_FitDisp(long pos, double eta, double eps, long nq,
				long q[], double dkL, long imax)
{
  /*pos : integer; eta, eps : double;
                         nq : integer; var q : fitvect;
                         dkL : double; imax : integer*/
  struct LOC_Ring_FitDisp V;

  int    i, j;
  double dkL1, Eta0, deta;
  bool   rad = false;

  if (setjmp(V._JL999)) goto _L999;

  if (trace)
    printf("  Dispersion fit, etax =%10.5f, eps =% .3E"
	   ", imax =%4ld, dkL =%10.5f\n",
	   eta, eps, imax, dkL);
  /* Turn off radiation */
  rad = Lattice.param.radiation; Lattice.param.radiation = false;
  Ring_GetTwiss(true, dP); checkifstable_(&V);
  Eta0 = Lattice.Cell[pos]->Eta[0];
  i = 0;
  while (fabs(eta-Eta0) > eps && i < imax) {
    i++;
    for (j = 0; j < nq; j++)
      shiftk_(q[j], dkL, &V);
    Ring_GetTwiss(true, dP); checkifstable_(&V);
    deta = Lattice.Cell[pos]->Eta[0] - Eta0;
    if (deta == 0.0) {
      printf("  deta is 0\n");
      goto _L999;
    }
    dkL1 = (eta-Eta0)*dkL/deta - dkL;
    for (j = 0; j < nq; j++)
      shiftk_(q[j], dkL1, &V);
    Ring_GetTwiss(true, dP); checkifstable_(&V);
    Eta0 = Lattice.Cell[pos]->Eta[0];
    if (trace)
      printf("  Dispersion = % .5E, kL =% .5E @%3d\n",
	     Eta0, Elem_GetKval(Lattice.Cell[q[0]]->Fnum, 1, (long)Quad), i);
  }
_L999:
  /* Restore radiation */
  Lattice.param.radiation = rad;
}
#undef dP
