/***********************************************************************

	File:	efst.c
	Rev:	b-2
	Date:	02/28/2001

	Copyright (c) 1998, 2001 by Pawel Winter, Martin Zachariasen
				    & David M. Warme

************************************************************************

	The main routine for the Euclidean FST generator.  It
	reads a point set from standard input, generates the FSTs,
	and outputs them to standard output.

************************************************************************

	Modification Log:

	a-1:	12/11/98	martinz
		: Created.  Derived from Pawel and Martin's C++ program
			    using Warme's infrastructure.
	b-1:	11/22/2000	martinz
		: Dynamic allocation of eq-point array using doubling.
		:  (-e option is now the INITIAL number of eq-points
		:  per terminal.)
		: Memory for rectangle structure freed when done.
		: Improved numerical robustness.
		: Translate points such that their mean is at the
		:  origin in order to compute eq-points with
		:  maximum precision.
		: Split off elementary geometric functions to efuncs.h.
		: Improved upper bound heuristic with BSD info.
		: Added new greedy heuristic.
		: Compute eq-points carefully by using displacements.
		: Compute distances between eq-points carefully by
		:  using displacements.
	b-2:	02/28/2001	warme
		: Use GMP, if available, to compute eq-points and
		:  EFST lengths precisely.

************************************************************************/

#include "bsd.h"
#include "config.h"
#include "efst.h"
#include "efuncs.h"
#include "egmp.h"
#include "p1io.h"
#include "steiner.h"


/*
 * Global Routines
 */

int			main (int, char **);

int			Multiple_Precision = 0;


/*
 * Local Routines
 */

static void		add_zero_length_fsts (struct einfo *, int, int **);
static void		build_fst_list (struct einfo *);
static void		build_efst_graph (struct einfo *,
					  struct point *,
					  struct eqp_t *,
					  struct point **,
					  struct edge **,
					  int *,
					  struct point *,
					  int *);
static void		compute_efsts_for_unique_terminals (struct einfo *);
static void		convert_delta_cpu_time (char *);
static void		decode_params (int, char **);
static int		generate_duplicate_terminal_groups (struct pset *,
							    int *,
							    int ***);
static cpu_time_t	get_delta_cpu_time (void);
static int *		heapsort_x (struct pset *);
static struct full_set ** put_trees_in_array (struct full_set *, int *);
static struct pset *	remove_duplicates (struct pset * pts,
					   int		 ndg,
					   int **	 list,
					   int **	 fwd_map,
					   int **	 rev_map);
static void		renumber_terminals (struct einfo *,
					    struct pset *,
					    int *);
static dist_t		test_and_save_fst (struct einfo *,
					   struct eqp_t *,
					   struct eqp_t *);
static void		usage (void);


/*
 * Local Variables
 */

static char *		description;
static int		InitialEqPointsTerminal = 100;
static int		EpsilonFactor = 32;
static char *		me;
static int		MaxFSTSize = 0;
static int		output_version = CURRENT_P1IO_VERSION;
static bool		Use_Greedy_Heuristic = FALSE;
static bool		Print_Detailed_Timings = FALSE;
static cpu_time_t	T0;
static cpu_time_t	Tn;


/*
 * Local Macros
 */

#define UPDATE_PTR(p,old,new) ((new) + ((p) - (old)))

#define UPDATE_RECTANGLE_BOUNDS(p) \
	{ *minx = MIN(*minx, p.x); *maxx = MAX(*maxx, p.x); \
	  *miny = MIN(*miny, p.y); *maxy = MAX(*maxy, p.y); }

/*
 * The main routine for the "efst" program.  It reads a point set
 * from standard input, generates all of the rectilinear FSTs,
 * and outputs them to standard output in our special "phase 1 I/O"
 * format.
 */

	int
main (

int		argc,
char **		argv
)
{
int			i;
int			j;
int			k;
int			ndg;
int			ntrees;
int			count;
int			fpsave;
int **			dup_grps;
int *			fwd_map;
int *			rev_map;
int *			ip1;
struct full_set *	fsp;
struct pset *		terms;
struct einfo		einfo;
struct cinfo		cinfo;
struct pset *		pts;
struct pset *		pts2;
cpu_time_t		Trenum;
struct scale_info	scale;
char			buf1 [32];

	fpsave = set_floating_point_double_precision ();

	setbuf (stdout, NULL);

	decode_params (argc, argv);

	pts = get_points (stdin, &scale);

	init_output_conversion (pts, EUCLIDEAN, &scale);

	T0 = get_cpu_time ();
	Tn = T0;

	einfo.x_order = heapsort_x (pts);

	if (Print_Detailed_Timings) {
		convert_delta_cpu_time (buf1);
		fprintf (stderr, "Sort X:                 %s\n", buf1);
	}

	/* Find all duplicate terminals in the input. */
	ndg = generate_duplicate_terminal_groups (pts,
						  einfo.x_order,
						  &dup_grps);

	einfo.num_term_masks = BMAP_ELTS (pts -> n);

	/* Remove all but the first of each duplicate terminal. */
	/* Compute forward and reverse maps to renumber the terminals. */
	pts2 = remove_duplicates (pts, ndg, dup_grps, &fwd_map, &rev_map);

	/* Renumber the x_order list -- instead of re-sorting pts2. */
	j = 0;
	for (i = 0; i < pts -> n; i++) {
		k = einfo.x_order [i];
		if ((k < 0) OR (pts -> n < k)) {
			fatal ("main: Bug 1.");
		}
		k = fwd_map [k];
		if (k < 0) continue;
		einfo.x_order [j++] = k;
	}

	if (Print_Detailed_Timings) {
		convert_delta_cpu_time (buf1);
		fprintf (stderr, "Remove Duplicates:      %s\n", buf1);
	}

	/* From now on, we work only with the reduced terminal set, and */
	/* we assume that all terminals are unique. */

	einfo.pts = pts2;

	compute_efsts_for_unique_terminals (&einfo);

	/* Now put the terminal numbers back the way they were, */
	/* renumber the terminals within each EFST, etc. */

	renumber_terminals (&einfo, pts, rev_map);

	/* Link the FSTs together into one long list, and number them. */
	build_fst_list (&einfo);

	/* Add one FST for each duplicate terminal that was removed. */
	if (ndg > 0) {
		add_zero_length_fsts (&einfo, ndg, dup_grps);
	}

	/* Measure renumber time.  This also sets Tn so that Tn-T0 is	*/
	/* the total processing time.					*/
	Trenum = get_delta_cpu_time ();

	if (Print_Detailed_Timings) {
		convert_cpu_time (Trenum, buf1);
		fprintf (stderr, "Renumber Terminals:     %s\n", buf1);
		convert_cpu_time (Tn - T0, buf1);
		fprintf (stderr, "Total:                  %s\n", buf1);
	}

	/* Zero out the compatibility info. */
	memset (&cinfo, 0, sizeof (cinfo));

	cinfo.num_edges			= einfo.ntrees;
	cinfo.num_verts			= einfo.pts -> n;
	cinfo.num_vert_masks		= BMAP_ELTS (cinfo.num_verts);
	cinfo.num_edge_masks		= BMAP_ELTS (cinfo.num_edges);
	cinfo.edge			= NEWA (einfo.ntrees + 1, int *);
	cinfo.edge_size			= NEWA (einfo.ntrees, int);
	cinfo.cost			= NEWA (einfo.ntrees, dist_t);
	cinfo.tflag			= NEWA (cinfo.num_verts, bool);
	cinfo.metric			= EUCLIDEAN;
	cinfo.scale			= scale;
	cinfo.integrality_delta		= 0;
	cinfo.pts			= einfo.pts;
	cinfo.full_trees		= put_trees_in_array (einfo.full_sets,
							      &ntrees);
	cinfo.mst_length		= einfo.mst_length;
	cinfo.description		= gst_strdup (description);
	cinfo.p1time			= Tn - T0;

	for (i = 0; i < cinfo.num_verts; i++) {
		cinfo.tflag [i] = TRUE;
	}

	count = 0;
	for (i = 0; i < einfo.ntrees; i++) {
		fsp = cinfo.full_trees [i];
		k = fsp -> terminals -> n;
		cinfo.edge_size [i]	= k;
		cinfo.cost [i]		= fsp -> tree_len;
		count += k;
	}
	ip1 = NEWA (count, int);
	for (i = 0; i < einfo.ntrees; i++) {
		cinfo.edge [i] = ip1;
		terms = cinfo.full_trees [i] -> terminals;
		k = terms -> n;
		for (j = 0; j < k; j++) {
			*ip1++ = terms -> a [j].pnum;
		}
	}
	cinfo.edge [i] = ip1;

	/* Print all of the data we have generated... */
	print_phase_1_data (&cinfo, output_version);

	/* Clean up. */

	free ((char *) pts2);
#if 0
	free ((char *) pts);
#endif

	free_phase_1_data (&cinfo);

	free ((char *) rev_map);
	free ((char *) fwd_map);
	if (dup_grps NE NULL) {
		if (dup_grps [0] NE NULL) {
			free ((char *) (dup_grps [0]));
		}
		free ((char *) dup_grps);
	}
	free ((char *) (einfo.x_order));

	restore_floating_point_precision (fpsave);

	exit (0);
}

/*
 * This routine decodes the various command-line arguments.
 */

	static
	void
decode_params (

int		argc,
char **		argv
)
{
char *		ap;
char		c;
int		v;

	--argc;
	me = *argv++;
	while (argc > 0) {
		ap = *argv++;
		if (*ap NE '-') {
			usage ();
		}
		++ap;
		while ((c = *ap++) NE '\0') {
			switch (c) {
			case 'd':
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				if (strlen (ap) >= 80) {
					fprintf (stderr,
						 "Description must be less"
						 " than 80 characters.\n");
					usage ();
				}
				description = ap;
				/* Change newlines to spaces... */
				for (;;) {
					ap = strchr (ap, '\n');
					if (ap EQ NULL) break;
					*ap++ = ' ';
				}
				ap = "";
				break;
			case 'e':
				/* Number of eq-points per terminal */
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				InitialEqPointsTerminal = atoi (ap);
				ap = "";
				break;

			case 'f':
				/* Epsilon multiplication factor */
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				EpsilonFactor = atoi (ap);
				ap = "";
				break;

			case 'g':
				Use_Greedy_Heuristic = TRUE;
				break;

			case 'k':
				/* Max number of terminals in FSTs */
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				MaxFSTSize = atoi (ap);
				ap = "";
				break;

#ifdef HAVE_GMP
			case 'm':
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				if ((*ap < '0') OR (*ap > '9')) {
					usage ();
				}
				Multiple_Precision = atoi (ap);
				ap = "";
				break;
#endif

			case 't':
				Print_Detailed_Timings = TRUE;
				break;

			case 'v':
				if (*ap EQ '\0') {
					if (argc <= 0) {
						usage ();
					}
					ap = *argv++;
					--argc;
				}
				if ((*ap < '0') OR (*ap > '9')) {
					usage ();
				}
				v = atoi (ap);
				if ((v < 0) OR (v > CURRENT_P1IO_VERSION)) {
					fprintf (stderr,
						 "%s: Bad version `%s'."
						 "  Valid versions range"
						 " from 0 to %d.\n",
						 me,
						 ap,
						 CURRENT_P1IO_VERSION);
					usage ();
				}
				output_version = v;
				ap = "";
				break;

			default:
				usage ();
				break;
			}
		}
		--argc;
	}
}

/*
 * This routine prints out the proper usage and exits.
 */

static char *	arg_doc [] = {
	"",
	"\t-d txt\tDescription of problem instance.",
	"\t-e K\tInitially allocate K eq-points per terminal",
	"\t\t(default: 100).",
	"\t-f E\tEpsilon multiplication factor for floating point number",
	"\t\tcomparisons (default: 32).",
	"\t-g\tUse greedy heuristic instead of Smith-Lee-Liebman",
	"\t\t(more time consuming but generates fewer eq-points).",
	"\t-k K\tOnly generate FSTs spanning up to K terminals.",
#ifdef HAVE_GMP
	"\t-m N\tUse multiple precision.  Larger N use it more.",
	"\t\t Default is N=0 which disables multiple precision.",
#endif
	"\t-t\tPrint detailed timings on stderr.",
	"\t-v N\tGenerates version N output data format.",
	"",
	NULL
};

	static
	void
usage (void)

{
char **		pp;
char *		p;

	(void) fprintf (stderr,
			"\nUsage: %s [-gt]"
			" [-d description]"
			" [-e K] [-f E] [-k K]"
#ifdef HAVE_GMP
			" [-m N]"
#endif
			" [-v N]"
			" <points-file\n",
			me);
	pp = &arg_doc [0];
	while ((p = *pp++) NE NULL) {
		(void) fprintf (stderr, "%s\n", p);
	}
	exit (1);
}

/*
 * Use the heapsort algorithm to sort the given terminals in increasing
 * order by the following keys:
 *
 *	1.	X coordinate
 *	2.	Y coordinate
 *	3.	index (i.e., position within input data)
 *
 * Of course, we do not move the points, but rather permute an array
 * of indexes into the points.
 */

	static
	int *
heapsort_x (

struct pset *		pts		/* IN - the terminals to sort */
)
{
int			i, i1, i2, j, k, n;
struct point *		p1;
struct point *		p2;
int *			index;

	n = pts -> n;

	index = NEWA (n, int);
	for (i = 0; i < n; i++) {
		index [i] = i;
	}

	/* Construct the heap via sift-downs, in O(n) time. */
	for (k = n >> 1; k >= 0; k--) {
		j = k;
		for (;;) {
			i = (j << 1) + 1;
			if (i + 1 < n) {
				/* Increment i (to right subchild of j) */
				/* if the right subchild is greater. */
				i1 = index [i];
				i2 = index [i + 1];
				p1 = &(pts -> a [i1]);
				p2 = &(pts -> a [i2]);
				if ((p2 -> x > p1 -> x) OR
				    ((p2 -> x EQ p1 -> x) AND
				     ((p2 -> y > p1 -> y) OR
				      ((p2 -> y EQ p1 -> y) AND
				       (i2 > i1))))) {
					++i;
				}
			}
			if (i >= n) {
				/* Hit bottom of heap, sift-down is done. */
				break;
			}
			i1 = index [j];
			i2 = index [i];
			p1 = &(pts -> a [i1]);
			p2 = &(pts -> a [i2]);
			if ((p1 -> x > p2 -> x) OR
			    ((p1 -> x EQ p2 -> x) AND
			     ((p1 -> y > p2 -> y) OR
			      ((p1 -> y EQ p2 -> y) AND
			       (i1 > i2))))) {
				/* Greatest child is smaller.  Sift-	*/
				/* down is done. */
				break;
			}
			/* Sift down and continue. */
			index [j] = i2;
			index [i] = i1;
			j = i;
		}
	}

	/* Now do actual sorting.  Exchange first/last and sift down. */
	while (n > 1) {
		/* Largest is at index [0], swap with index [n-1],	*/
		/* thereby putting it into final position.		*/
		--n;
		i = index [0];
		index [0] = index [n];
		index [n] = i;

		/* Now restore the heap by sifting index [0] down. */
		j = 0;
		for (;;) {
			i = (j << 1) + 1;
			if (i + 1 < n) {
				/* Increment i (to right subchild of j) */
				/* if the right subchild is greater. */
				i1 = index [i];
				i2 = index [i + 1];
				p1 = &(pts -> a [i1]);
				p2 = &(pts -> a [i2]);
				if ((p2 -> x > p1 -> x) OR
				    ((p2 -> x EQ p1 -> x) AND
				     ((p2 -> y > p1 -> y) OR
				      ((p2 -> y EQ p1 -> y) AND
				       (i2 > i1))))) {
					++i;
				}
			}
			if (i >= n) {
				/* Hit bottom of heap, sift-down is done. */
				break;
			}
			i1 = index [j];
			i2 = index [i];
			p1 = &(pts -> a [i1]);
			p2 = &(pts -> a [i2]);
			if ((p1 -> x > p2 -> x) OR
			    ((p1 -> x EQ p2 -> x) AND
			     ((p1 -> y > p2 -> y) OR
			      ((p1 -> y EQ p2 -> y) AND
			       (i1 > i2))))) {
				/* Greatest child is smaller.  Sift-	*/
				/* down is done. */
				break;
			}
			/* Sift down and continue. */
			index [j] = i2;
			index [i] = i1;
			j = i;
		}
	}

	return (index);
}

/*
 * This routine finds all pairs of points whose coordinates are exactly
 * identical.  This is a degeneracy that can cause successive algorithms
 * much heartburn, so we take care of them immediately by keeping only
 * the earliest (lowest pnum) point, and marking the others as duplicates.
 * We generate a partition of such terminals into subsets -- if terminals
 * A and B reside in the same subset then they have identical coordinates.
 * We refer to such a subset as a Duplicate Terminal Group.
 *
 * For each terminal group we retain ONLY THE FIRST member -- the terminal
 * map bits are turned OFF for all other members of a terminal group,
 * effectively eliminating those terminals from the problem.
 */

	static
	int
generate_duplicate_terminal_groups (

struct pset *		pts,	/* IN - original terminal set */
int *			xorder, /* IN - terminal numbers sorted by X coord */
int ***			grps	/* OUT - duplicate terminal groups */
)
{
int			i;
int			j;
int			n;
int			n_grps;
struct point *		p0;
struct point *		p1;
struct point *		p2;
int *			ip;
int **			real_ptrs;
int *			real_terms;
int **			ptrs;
int *			terms;

	n = pts -> n;

	n_grps = 0;

	ptrs	= NEWA (n + 1, int *);
	terms	= NEWA (n, int);

	ip = &terms [0];
	for (i = 1; i < n; ) {
		p0 = &(pts -> a [xorder [i - 1]]);
		p1 = &(pts -> a [xorder [i]]);

		if ((p0 -> y NE p1 -> y) OR (p0 -> x NE p1 -> x)) {
			/* Not identical. */
			++i;
			continue;
		}

		/* Terminals xorder[i-1] and xorder[i] are identical. */

		for (j = i + 1; j < n; j++) {
			p2 = &(pts -> a [xorder [j]]);
			if (p0 -> y NE p2 -> y) break;
			if (p0 -> x NE p2 -> x) break;
		}
		/* j now points to the first non-equal terminal */

		/* Make a new duplicate terminal group... */
		ptrs [n_grps++] = ip;
		*ip++ = xorder [i - 1];
		while (i < j) {
			*ip++ = xorder [i++];
		}

		/* Skip whole group of coincident points and continue */
	}
	ptrs [n_grps] = ip;

	if (n_grps <= 0) {
		*grps = NULL;
	}
	else {
		/* Transfer to permanent memory of proper size... */
		real_ptrs = NEWA (n_grps + 1, int *);
		real_terms = NEWA (ip - terms, int);
		(void) memcpy ((char *) real_terms,
			       (char *) terms,
			       (ip - terms) * sizeof (int));
		for (i = 0; i <= n_grps; i++) {
			real_ptrs [i] = &real_terms [ptrs [i] - ptrs [0]];
		}
		*grps = real_ptrs;
	}

	free ((char *) terms);
	free ((char *) ptrs);

	return (n_grps);
}

/*
 * This routine removes all but the first terminal in each duplicate
 * terminal group.  We also prepare arrays for mapping forward from
 * old to new terminal numbers, and backward from new terminal numbers
 * to old.
 */

	static
	struct pset *
remove_duplicates (

struct pset *		pts,		/* IN - original point set */
int			ndg,		/* IN - number of duplicate groups */
int **			list,		/* IN - list of duplicate groups */
int **			fwd_map_ptr,	/* OUT - map to renumber old to new */
int **			rev_map_ptr	/* OUT - map to renumber new to old */
)
{
int		i;
int		j;
int		n;
int		kmasks;
int		numdel;
int		new_n;
int *		ip1;
int *		ip2;
int *		fwd;
int *		rev;
bitmap_t *	deleted;
struct pset *	newpts;

	n = pts -> n;

	kmasks = BMAP_ELTS (n);
	deleted = NEWA (kmasks, bitmap_t);
	for (i = 0; i < kmasks; i++) {
		deleted [i] = 0;
	}

	numdel = 0;
	for (i = 0; i < ndg; i++) {
		ip1 = list [i];
		ip2 = list [i + 1];

		/* Retain the first in this group, exclude all	*/
		/* of the others...				*/
		while (++ip1 < ip2) {
			if (BITON (deleted, *ip1)) {
				fatal ("remove_duplicates: Bug 1.");
			}
			++numdel;
			SETBIT (deleted, *ip1);
		}
	}

	new_n = n - numdel;

	fwd = NEWA (n, int);
	rev = NEWA (new_n, int);
	newpts = NEW_PSET (new_n);
	ZERO_PSET (newpts, new_n);
	newpts -> n = new_n;
	j = 0;
	for (i = 0; i < n; i++) {
		if (BITON (deleted, i)) {
			fwd [i] = -1;
		}
		else {
			newpts -> a [j].x		= pts -> a [i].x;
			newpts -> a [j].y		= pts -> a [i].y;
			newpts -> a [j].pnum		= j;
			rev [j] = i;
			fwd [i] = j;
			++j;
		}
	}

	free ((char *) deleted);

	*fwd_map_ptr = fwd;
	*rev_map_ptr = rev;

	return (newpts);
}


/* Solve a quadratic equation of the form
 * A/2 X^2 + B X + C/2 = 0
 *
 * We use a numerically robust formula from Press et al: Numerical
 * Recipies.
 * Only solves the equation if A <> 0.
 * Return FALSE if there are no real roots.
 */

	static
	bool
solve_quadratic (

double	 A,	 /* IN - twice the first coefficient */
double	 B,	 /* IN - second coefficient */
double	 C,	 /* IN - twice the third coefficient */
double * root1,	 /* OUT - first root */
double * root2	 /* OUT - second root */
)
{
	double Q, D;

	if (A EQ 0.0) return FALSE; /* this is not a quadratic equation */

	D = B*B - A*C;
	if (D < 0.0)  return FALSE; /* no real roots */

	D = sqrt(D);
	if (B >= 0.0)
		Q = - B - D;
	else
		Q = - B + D;

	if (Q NE 0.0) {
		*root1 = Q / A;
		*root2 = C / Q;
	}
	else {
		*root1 = 0.0;
		*root2 = -2.0 * B / A;
	}

	return TRUE;
}

/*
 * Upper bound heuristic (just calls the heuristic that was chosen)
 */

	static
	dist_t
upper_bound_heuristic (

struct pset *	pts,	/* IN - point set */
struct bsd  *	bsdp	/* IN - BSD info (can be NULL) */
)
{
	if (NOT Use_Greedy_Heuristic) {
		return smith_lee_liebman (pts, bsdp);
	}
	else {
		return greedy_heuristic	 (pts, bsdp);
	}
}

/*
 * Test if a new value of LP leads to a pruning of the eq-point.
 * If not then update LP.
 * First move the new point slightly to the left in order to
 * avoid (too many) numerical difficulties.
 */

	static
	bool
test_and_save_LP (

struct einfo *	eip,	/* IN - global EFST info */
struct eqp_t *	eqpi,	/* IN - first eq-point */
struct eqp_t *	eqpj,	/* IN - second eq-point */
struct eqp_t *	eqpk,	/* IN/OUT - new eq-point */
struct point *	CLP	/* IN - new proposed LP point */
)
{
struct point NLP;
struct point PLP;

	/* First check that the new point is between eqpk -> LP and eqpi -> E */
	if ((CLP -> x EQ eqpi -> E.x) AND (CLP -> y EQ eqpi -> E.y)) return TRUE;
	if (right_turn(&(eqpk -> E), &(eqpi -> E),  CLP))	     return TRUE;
	if (left_turn (&(eqpk -> E), &(eqpk -> LP), CLP))	     return TRUE;

	memset (&PLP, 0, sizeof (PLP));
	memset (&NLP, 0, sizeof (NLP));

	/* Now move it a little closer to eqpj -> E, just to be on the safe side */
	PLP.x = CLP -> x + (eqpj -> E.x - CLP -> x) * eip -> eps * ((double) eqpk -> S);
	PLP.y = CLP -> y + (eqpj -> E.y - CLP -> y) * eip -> eps * ((double) eqpk -> S);
	project_point(&(eqpk -> E), &(eqpk -> DC), &PLP, &NLP);

	/* We pass eqpk -> LP? */
	if (left_turn (&(eqpk -> E), &(eqpk -> LP), &NLP))	     return TRUE;

	/* Now make the actual testing */
	if (right_turn(&(eqpk -> E), &(eqpk -> RP), &NLP))	     return FALSE;
	eqpk -> LP = NLP;
	return TRUE;
}

/*
 * Test if a new value of RP leads to a pruning of the eq-point.
 * If not then update RP.
 * First move the new point slightly to the right in order to
 * avoid (too many) numerical difficulties.
 */

	static
	bool
test_and_save_RP (

struct einfo *	eip,	/* IN - global EFST info */
struct eqp_t *	eqpi,	/* IN - first eq-point */
struct eqp_t *	eqpj,	/* IN - second eq-point */
struct eqp_t *	eqpk,	/* IN/OUT - new eq-point */
struct point *	CRP	/* IN - new proposed RP point */
)
{
struct point NRP;
struct point PRP;

	/* First check that the new point is between eqpj -> E and eqpk -> RP */
	if ((CRP -> x EQ eqpj -> E.x) AND (CRP -> y EQ eqpj -> E.y)) return TRUE;
	if (left_turn (&(eqpk -> E), &(eqpj -> E),  CRP))	     return TRUE;
	if (right_turn(&(eqpk -> E), &(eqpk -> RP), CRP))	     return TRUE;

	memset (&PRP, 0, sizeof (PRP));
	memset (&NRP, 0, sizeof (NRP));

	/* Now move it a little closer to eqpi -> E, just to be on the safe side */
	PRP.x = CRP -> x + (eqpi -> E.x - CRP -> x) * eip -> eps * ((double) eqpk -> S);
	PRP.y = CRP -> y + (eqpi -> E.y - CRP -> y) * eip -> eps * ((double) eqpk -> S);
	project_point(&(eqpk -> E), &(eqpk -> DC), &PRP, &NRP);

	/* We pass eqpk -> RP? */
	if (right_turn (&(eqpk -> E), &(eqpk -> RP), &NRP))	     return TRUE;

	/* Now make the actual testing */
	if (left_turn(&(eqpk -> E), &(eqpk -> LP), &NRP))	     return FALSE;
	eqpk -> RP = NRP;
	return TRUE;
}

/*
 * Flag all terminals involved in eq-point k
 */

	static
	void
set_member_arr (

struct einfo * eip,  /* IN/OUT - global EFST info */
struct eqp_t * eqp,  /* IN - eq-point */
bool flag	     /* IN - value of flag */
)
{
	eterm_t *p    = eqp -> Z;
	eterm_t *endp = p + eqp -> S;

	while (p < endp) {
		int t = *p++;
		eip -> MEMB [t] = flag;
	}
}

/*
 * Merge two disjoint ordered lists of terminal numbers.  The result
 * is of course also ordered.
 */

	static
	eterm_t *
merge_terminal_lists (

struct einfo * eip,  /* IN/OUT - global EFST info */
struct eqp_t * eqpi, /* IN - first eq-point */
struct eqp_t * eqpj, /* IN - second eq-point */
struct eqp_t * eqpk  /* IN - new eq-point */
)
{
	eterm_t *p1, *endp1, *p2, *endp2, *Zp, *eqpZ_old;
	int t1, t2;
	struct eqp_t *eqpt;

	/* Set new eq-point terminal list pointer */
	eqpk -> Z = eip -> eqpZ_curr;

	if (eip -> eqpZ_curr + eqpi -> S + eqpj -> S
		>  eip -> eqpZ + eip -> eqpZ_size)  {

		/* Terminal list space exhausted - double array */

		if (Print_Detailed_Timings)
			fprintf(stderr, "- doubling terminal list array\n");
		eqpZ_old = eip -> eqpZ;
		eip -> eqpZ = NEWA ( eip -> eqpZ_size * 2, eterm_t );
		memcpy ( eip -> eqpZ, eqpZ_old, eip -> eqpZ_size * sizeof(eterm_t) );
		eip -> eqpZ_size = 2 * eip -> eqpZ_size;
		eip -> eqpZ_curr = UPDATE_PTR( eip -> eqpZ_curr, eqpZ_old, eip -> eqpZ );

		/* Update pointers from eq-point array */
		for (eqpt = eip -> eqp; eqpt <= eqpk; eqpt++)
			eqpt -> Z = UPDATE_PTR( eqpt -> Z, eqpZ_old, eip -> eqpZ );
		free( eqpZ_old );
	}

	p1 = eqpi -> Z;
	p2 = eqpj -> Z;
	endp1 = p1 + eqpi -> S;
	endp2 = p2 + eqpj -> S;

	/* We know each list contains at least one item! */
	t1 = *p1++;
	t2 = *p2++;

	/* I tried it lots of different ways and discovered that
	   these goto's actually produce the MOST readable form! */
	Zp = eip -> eqpZ_curr;
	for (;;) {
		if (t1 < t2) {
			*Zp++ = t1;
			if (p1 >= endp1) goto finish2;
			t1 = *p1++;
		}
		else {
			/* Disjoint implies t2 < t1 right here. */
			*Zp++ = t2;
			if (p2 >= endp2) goto finish1;
			t2 = *p2++;
		}
	}

finish1:
	for (;;) {
		*Zp++ = t1;
		if (p1 >= endp1) break;
		t1 = *p1++;
	}
	return (Zp);

finish2:
	for (;;) {
		*Zp++ = t2;
		if (p2 >= endp2) break;
		t2 = *p2++;
	}
	return (Zp);
}

/*
 * Test if terminals involved in eq-point j are disjoint from those
 * already flagged.
 */

	static
	bool
disjoint (

struct einfo * eip, /* IN - global EFST info */
struct eqp_t * eqp  /* IN - eq-point */
)
{
	eterm_t *p = eqp -> Z;
	eterm_t *endp = p + eqp -> S;

	while (p < endp) {
		int t = *p++;
		if (eip -> MEMB [t]) return FALSE;
	}
	return TRUE;
}

/*
 * Return terminal with highest index involved in eq-point k
 */

	static
	int
highest_terminal (

struct eqp_t * eqp  /* IN - eq-point */
)
{
	return ( (eqp -> Z) [eqp -> S  -  1]);
}


/*
 * Return the closest terminal to Q involved in the construction of
 * equilateral point number k
 */

	static
	dist_t
closest_terminal (

struct einfo * eip,  /* IN - global EFST info */
struct point * Q,    /* IN - query point */
struct eqp_t * eqpk  /* IN - eq-point */
)
{
	eterm_t *p, *endp;
	int t;
	dist_t min_dist2, dist2;
	int min_t;

	p = eqpk -> Z;
	endp = p + eqpk -> S;

	min_t = *p++;
	min_dist2 = sqr_dist(Q, &(eip -> eqp[min_t].E));
	while (p < endp) {
		t = *p++;
		dist2 = sqr_dist(Q, &(eip -> eqp[t].E));
		if (dist2 < min_dist2) {
			min_dist2 = dist2;
			min_t = t;
		}
	}

	return (sqrt (min_dist2));
}

/*
 * Get bottleneck Steiner distance between equilateral points i and j
 */

	static
	double
getBSD (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpi, /* IN - first eq-point */
struct eqp_t * eqpj  /* IN - second eq-point */
)
{
	dist_t rbsd, lbsd;
	if (eqpi -> R EQ NULL) {
		if (eqpj -> R EQ NULL) return bsd(eip -> bsd, eqpi -> E.pnum, eqpj -> E.pnum);
		rbsd = getBSD(eip, eqpi, eqpj -> R);
		lbsd = getBSD(eip, eqpi, eqpj -> L);
	}
	else {
		rbsd = getBSD(eip, eqpi -> R, eqpj);
		lbsd = getBSD(eip, eqpi -> L, eqpj);
	}
	if (rbsd < lbsd) return rbsd;
	return lbsd;
}


/*
 * Computation of eq-point displacement vector
 */

	static
	void
eq_point_disp_vector (

struct einfo * eip,   /* IN - global EFST info */
struct eqp_t * eqpi,  /* IN - first eq-point */
struct eqp_t * eqpj,  /* IN - second eq-point */
struct eqp_t * eqpk   /* IN - new eq-point */
)
{
pnum_t		ti, tj;
double		dx, dy, rdx, rdy;

	/* First compute vector between old eq-points terminal references */
	ti = eqpi -> DV.pnum;
	tj = eqpj -> DV.pnum;

	dx = eip -> eqp [tj].E.x - eip -> eqp [ti].E.x;
	dy = eip -> eqp [tj].E.y - eip -> eqp [ti].E.y;

	/* Add displacements */
	dx -= eqpi -> DV.x;
	dy -= eqpi -> DV.y;
	dx += eqpj -> DV.x;
	dy += eqpj -> DV.y;

	/* Rotate 60 degrees and save */
	rotate60(dx, dy, &rdx, &rdy);
	eqpk -> DV.x	= rdx + eqpi -> DV.x;
	eqpk -> DV.y	= rdy + eqpi -> DV.y;
	eqpk -> DV.pnum = eqpi -> DV.pnum;
}

/*
 * Computation of distance between eq-points.
 * We do this in a numerically careful way by computing displacements
 * and not absolute coordinates.
 */

	static
	dist_t
eq_point_dist (

struct einfo * eip,   /* IN - global EFST info */
struct eqp_t * eqpi,  /* IN - first eq-point */
struct eqp_t * eqpj   /* IN - second eq-point */
)
{
pnum_t		ti, tj;
double		dx, dy, rdx, rdy;

	/* First compute vector between eq-points terminal references */
	ti = eqpi -> DV.pnum;
	tj = eqpj -> DV.pnum;

	dx = eip -> eqp [tj].E.x - eip -> eqp [ti].E.x;
	dy = eip -> eqp [tj].E.y - eip -> eqp [ti].E.y;

	/* Add displacements */
	dx -= eqpi -> DV.x;
	dy -= eqpi -> DV.y;
	dx += eqpj -> DV.x;
	dy += eqpj -> DV.y;

	/* Finally return length of vector */
	return hypot (dx, dy);
}

/*
 * List of terminals involved in the construction of eq-point
 */

	static
	void
eqpoint_terminals (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpk, /* IN - eq-point */
struct pset  * pts,  /* OUT - list of point */
bool   append	     /* IN - append to list? */
)
{
	eterm_t *p = eqpk -> Z;
	eterm_t *endp = p + eqpk -> S;
	struct point *pt;

	if (NOT append) pts -> n = 0;

	pt = pts -> a + pts -> n;
	while (p < endp)
		*(pt++) = eip -> eqp[ *p++ ].E;
	pts -> n += eqpk -> S;
}

/*
 * Initialize data structure for eq-point rectangles.
 * We use a hashing like data structure; the rectangle enclosing all
 * the terminals is divided into squares with side length of the
 * longest MST edge. For each such square the list of eq-point
 * rectangles overlapping with that square are stored in an array.

 * The idea of using eq-point rectangles was proposed by Ernst Althaus,
 * Max-Planck-Institut fur Informatik, Germany.
 * He used a 2D-search trees to store the rectangles; we apply
 * a simpler alternative here.
 */

	static
	void
initialize_eqp_rectangles (

struct einfo * eip /* IN/OUT - global EFST info */
)
{
	int i;
	struct point * p;

	/* Allocate square data structure */
	eip -> eqp_squares = NEWA(eip -> pts -> n, struct eqp_square_t *);

	/* Find range of terminal coordinates */
	eip -> minx =  INF_DISTANCE;
	eip -> maxx = -INF_DISTANCE;
	eip -> miny =  INF_DISTANCE;
	eip -> maxy = -INF_DISTANCE;
	for (i = 0; i < eip -> pts -> n; i++) {
		p = &(eip -> pts -> a[i]);
		eip -> minx = MIN( eip -> minx, p -> x - eip -> mean.x);
		eip -> maxx = MAX( eip -> maxx, p -> x - eip -> mean.x);
		eip -> miny = MIN( eip -> miny, p -> y - eip -> mean.y);
		eip -> maxy = MAX( eip -> maxy, p -> y - eip -> mean.y);
		eip -> eqp_squares[i] = NULL;
	}

	/* Set up basic paramaters */
	eip -> eqp_square_size = eip -> max_mst_edge;
	eip -> srangex = floor( (eip -> maxx - eip -> minx) / eip -> eqp_square_size) + 1;
	eip -> srangey = floor( (eip -> maxy - eip -> miny) / eip -> eqp_square_size) + 1;
}


/*
 * Free memory used by eq-point rectangle data structure.
 */

	static
	void
destroy_eqp_rectangles (

struct einfo * eip /* IN/OUT - global EFST info */
)
{
	int i;

	for (i = 0; i < eip -> pts -> n; i++)
	if (eip -> eqp_squares[i] NE NULL) {
		free( eip -> eqp_squares[i][0].eqp ); /* eq-point lists */
		free( eip -> eqp_squares[i]	   ); /* pointers to eq-point lists */
	}

	free( eip -> eqp_squares );
}

/*
 * Compute boundary of rectangle in which the Steiner point joining
 * the current eq-point to another eq-point can be placed
 */

	static
	void
compute_eqp_rectangle (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpk, /* IN - eq-point */
dist_t * minx,	     /* OUT - minimum x-coordinate of boundary */
dist_t * maxx,	     /* OUT - maximum x-coordinate of boundary */
dist_t * miny,	     /* OUT - minimum y-coordinate of boundary */
dist_t * maxy	     /* OUT - maximum x-coordinate of boundary */
)
{
	struct point p;
	dist_t alpha;

	if (eqpk -> S EQ 1) {

		/* This is a terminal */
		*minx = eqpk -> E.x - eip -> max_mst_edge;
		*maxx = eqpk -> E.x + eip -> max_mst_edge;
		*miny = eqpk -> E.y - eip -> max_mst_edge;
		*maxy = eqpk -> E.y + eip -> max_mst_edge;
	}
	else {

		/* This is a "normal" eq-point */
		*minx = eqpk -> LP.x; *maxx = eqpk -> LP.x;
		*miny = eqpk -> LP.y; *maxy = eqpk -> LP.y;
		UPDATE_RECTANGLE_BOUNDS(eqpk -> RP);

		/* Projection of eqpk -> LP (from eqpk -> E) to a point
		   at distance max_mst_edge away. */
		alpha = 1.0 + eip -> max_mst_edge / EDIST(&(eqpk -> E), &(eqpk -> LP));
		p.x   = eqpk -> E.x + alpha * (eqpk -> LP.x - eqpk -> E.x);
		p.y   = eqpk -> E.y + alpha * (eqpk -> LP.y - eqpk -> E.y);
		UPDATE_RECTANGLE_BOUNDS(p);

		/* Projection of eqpk -> RP (from eqpk -> E) to a point
		   at distance max_mst_edge away. */
		alpha = 1.0 + eip -> max_mst_edge / EDIST(&(eqpk -> E), &(eqpk -> RP));
		p.x   = eqpk -> E.x + alpha * (eqpk -> RP.x - eqpk -> E.x);
		p.y   = eqpk -> E.y + alpha * (eqpk -> RP.y - eqpk -> E.y);
		UPDATE_RECTANGLE_BOUNDS(p);

		/* Now check all intersections with axis parallel lines
		   through  eqpk -> DC */
		p.x = eqpk -> DC.x + eqpk -> DR + eip -> max_mst_edge;
		p.y = eqpk -> DC.y;
		if (right_turn(&(eqpk -> E), &(eqpk -> LP), &p) AND
		    left_turn(&(eqpk -> E), &(eqpk -> RP), &p))
			UPDATE_RECTANGLE_BOUNDS(p);

		p.x = eqpk -> DC.x - eqpk -> DR - eip -> max_mst_edge;
		p.y = eqpk -> DC.y;
		if (right_turn(&(eqpk -> E), &(eqpk -> LP), &p) AND
		    left_turn(&(eqpk -> E), &(eqpk -> RP), &p))
			UPDATE_RECTANGLE_BOUNDS(p);

		p.x = eqpk -> DC.x;
		p.y = eqpk -> DC.y + eqpk -> DR + eip -> max_mst_edge;
		if (right_turn(&(eqpk -> E), &(eqpk -> LP), &p) AND
		    left_turn(&(eqpk -> E), &(eqpk -> RP), &p))
			UPDATE_RECTANGLE_BOUNDS(p);

		p.x = eqpk -> DC.x;
		p.y = eqpk -> DC.y - eqpk -> DR - eip -> max_mst_edge;
		if (right_turn(&(eqpk -> E), &(eqpk -> LP), &p) AND
		    left_turn(&(eqpk -> E), &(eqpk -> RP), &p))
			UPDATE_RECTANGLE_BOUNDS(p);
	}

	*minx -= (fabs(*minx) * eip -> eps * ((double) eqpk -> S));
	*miny -= (fabs(*miny) * eip -> eps * ((double) eqpk -> S));
	*maxx += (fabs(*maxx) * eip -> eps * ((double) eqpk -> S));
	*maxy += (fabs(*maxy) * eip -> eps * ((double) eqpk -> S));
}

/*
 * Save all eq-point rectangles for a given eq-point size
 */

	static
	void
save_eqp_rectangles (

struct einfo * eip,  /* IN/OUT - global EFST info */
int first_eqp,	     /* IN - index of first eq-point to be saved */
int last_eqp	     /* IN - index of last eq-point to be saved */
)
{
	int i, j, k, si, size;
	struct eqp_t * eqpk;
	dist_t minx, maxx, miny, maxy;
	int total_squares;
	int * curr_count;
	struct eqp_square_t* sqr;
	struct eqp_t ** eqp_alloc;

	if (first_eqp > last_eqp) return; /* no eq-points to save */

	/* Go through all equilateral points.
	 * Find min/max square indices in each dimension and count the total
	 * number of squares that are overlapped by these eq-points.
	 */

	size = eip -> eqp[first_eqp].S;
	total_squares = 0;
	for (k = first_eqp; k <= last_eqp; k++) {
		eqpk = &(eip -> eqp[k]);

		/* Compute geometric range of rectangle for this eq-point */
		compute_eqp_rectangle(eip, eqpk, &minx, &maxx, &miny, &maxy);

		/* Compute square index range for this eq-point */

		eqpk -> SMINX = MAX(0,		      floor( (minx - eip -> minx) /
						       eip -> eqp_square_size));
		eqpk -> SMAXX = MIN(eip -> srangex-1, floor( (maxx - eip -> minx) /
						       eip -> eqp_square_size));
		eqpk -> SMINY = MAX(0,		      floor( (miny - eip -> miny) /
						       eip -> eqp_square_size));
		eqpk -> SMAXY = MIN(eip -> srangey-1, floor( (maxy - eip -> miny) /
						       eip -> eqp_square_size));

		total_squares += (eqpk -> SMAXX - eqpk -> SMINX + 1) *
				 (eqpk -> SMAXY - eqpk -> SMINY + 1);
	}

	/* Allocate space for these rectangles */
	eip -> eqp_squares[size] = NEWA(eip -> srangex * eip -> srangey,
						struct eqp_square_t);
	sqr = eip -> eqp_squares[size];

	/* Initialize eq-point counts */
	curr_count = NEWA(eip -> srangex * eip -> srangey, int);
	for (i = 0; i < eip -> srangex; i++)
		for (j = 0; j < eip -> srangey; j++) {
			si = i * eip -> srangey + j;
			sqr[si].n = 0; curr_count[si] = 0;
		}

	/* Find counts for each square */
	for (k = first_eqp; k <= last_eqp; k++) {
		eqpk = &(eip -> eqp[k]);

		for (i = eqpk -> SMINX; i <= eqpk -> SMAXX; i++)
			for (j = eqpk -> SMINY; j <= eqpk -> SMAXY; j++)
				sqr[ i * eip -> srangey + j ].n++;
	}

	/* Set up pointers */
	eqp_alloc = NEWA(total_squares, struct eqp_t *);
	for (i = 0; i < eip -> srangex; i++)
		for (j = 0; j < eip -> srangey; j++) {
			si = i * eip -> srangey + j;
			sqr[si].eqp = eqp_alloc; eqp_alloc += sqr[si].n;
		}

	/* Fill arrays */
	for (k = first_eqp; k <= last_eqp; k++) {
		eqpk = &(eip -> eqp[k]);

		for (i = eqpk -> SMINX; i <= eqpk -> SMAXX; i++)
			for (j = eqpk -> SMINY; j <= eqpk -> SMAXY; j++) {
				si = i * eip -> srangey + j;
				sqr[si].eqp[ curr_count[si]++ ] = eqpk;
			}
	}

	free(curr_count);
}


/*
 * Generate compatible eq-points, i.e., eq-points that are
 * close enough to be joined to a given eq-point.
 */

	static
	void
generate_compatible_eqp (

struct einfo * eip,	  /* IN - global EFST info */
int size,		  /* IN - prespecified size of eq-points */
struct eqp_t * eqpi,	  /* IN - given eq-point */
struct eqp_t ** eqp_list  /* OUT - list of compatible eq-points */
)
{
	struct eqp_square_t * sqr;
	int i, j, l, si;
	struct eqp_t ** eqpp;
	struct eqp_t *	eqpj;

	sqr  = eip -> eqp_squares[size];
	eqpp = eqp_list;

	if (sqr) {
		for (i = eqpi -> SMINX; i <= eqpi -> SMAXX; i++)
			for (j = eqpi -> SMINY; j <= eqpi -> SMAXY; j++) {
				si = i * eip -> srangey + j;
				for (l = 0; l < sqr[si].n; l++) {
					eqpj = sqr[si].eqp[l];
					if (NOT (eqpj -> CHOSEN)) {
						*(eqpp++) = eqpj;
						eqpj -> CHOSEN = TRUE;
					}
				}
			}
	}
	*eqpp = NULL;
}



/*
 * Basic projection test (case I)
 */

	static
	bool
projection_test_case_I (

struct einfo * eip,   /* IN - global EFST info */
struct eqp_t * eqpi,  /* IN - first eq-point */
struct eqp_t * eqpj   /* IN - second eq-point */
)
{
	if (eqpi -> R) {
		get_angle_vector(&(eqpi -> RP), &(eqpi -> E), &(eqpj -> E),
				 &(eip -> dxi), &(eip -> dyi));
		if (angle_geq_120(eip -> dxi, eip -> dyi))
			return FALSE;
	}

	if (eqpj -> R) {
		get_angle_vector(&(eqpi -> E), &(eqpj -> E), &(eqpj -> LP),
				 &(eip -> dxj), &(eip -> dyj));
		if (angle_geq_120(eip -> dxj, eip -> dyj))
			return FALSE;
	}

	return TRUE;
}

/*
 * Projection test (cases II - VI)
 */

	static
	bool
projection_test_cases_II_VI (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpi, /* IN - first eq-point */
struct eqp_t * eqpj, /* IN - second eq-point */
struct eqp_t * eqpk  /* IN - new eq-point */
)
{
	struct point CLP, CRP;

	if (eqpi -> R) {
		if (angle_geq_60(eip -> dxi, eip -> dyi)) {
			if (sqr_dist(&(eqpk -> DC), &(eqpi -> LP)) >= eqpk -> DR2)
				return FALSE;				     /* case II	 */
			project_point(&(eqpi -> E),  &(eqpk -> DC),
				      &(eqpi -> LP), &(eqpk -> LP));	     /* case III */
			project_point_perp(&(eqpi -> E),  &(eqpk -> DC),
					   &(eqpi -> DC), &(eqpk -> RP));
		}
		else {
			if (sqr_dist(&(eqpi -> DC), &(eqpk -> LP)) <= eqpi -> DR2)
				return FALSE;				     /* case IV	 */
			if (sqr_dist(&(eqpk -> DC), &(eqpi -> RP)) >= eqpk -> DR2) {
				if (sqr_dist(&(eqpk -> DC), &(eqpi -> LP)) >= eqpk -> DR2)
					return FALSE;			     /* case V	 */
				project_point_perp(&(eqpi -> E),  &(eqpk -> DC),
						   &(eqpi -> DC), &(eqpk -> RP));
			}
			else
				project_point(&(eqpi -> E),  &(eqpk -> DC),
					      &(eqpi -> RP), &(eqpk -> RP)); /* case VI	 */

			if (right_turn(&(eqpi -> E), &(eqpk -> LP), &(eqpi -> LP)))
				project_point(&(eqpi -> E),  &(eqpk -> DC),
					      &(eqpi -> LP), &(eqpk -> LP));
		}
	}


	if (eqpj -> R) {
		memset (&CLP, 0, sizeof (CLP));
		memset (&CRP, 0, sizeof (CRP));
		if (angle_geq_60(eip -> dxj, eip -> dyj)) {
			if (sqr_dist(&(eqpk -> DC), &(eqpj -> RP)) >= eqpk -> DR2)
				return FALSE;				     /* case II	 */
			project_point(&(eqpj -> E), &(eqpk -> DC),
				      &(eqpj -> RP), &CRP);		     /* case III */
			if (right_turn(&(eqpk -> E), &CRP, &(eqpk -> LP)))
				return FALSE;
			if (right_turn(&(eqpk -> E), &CRP, &(eqpk -> RP)))
				eqpk -> RP = CRP;
			project_point_perp(&(eqpj -> E),  &(eqpk -> DC),
					   &(eqpj -> DC), &CLP);
			if (right_turn(&(eqpk -> E), &(eqpk -> RP), &CLP))
				return FALSE;
			if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLP))
				eqpk -> LP = CLP;
		}
		else {
			if (sqr_dist(&(eqpj -> DC), &(eqpk -> RP)) <= eqpj -> DR2)
				return FALSE;				     /* case IV */
			if (sqr_dist(&(eqpk -> DC), &(eqpj -> LP)) >= eqpk -> DR2)  {
				if (sqr_dist(&(eqpk -> DC), &(eqpj -> RP)) >= eqpk -> DR2)
					return FALSE;			     /* case V	*/
				project_point_perp(&(eqpj -> E),  &(eqpk -> DC),
						   &(eqpj -> DC), &CLP);
			}
			else
				project_point(&(eqpj -> E),  &(eqpk -> DC),
					      &(eqpj -> LP), &CLP);	     /* case VI */

			if (right_turn(&(eqpk -> E), &(eqpk -> RP), &CLP))
				return FALSE;
			if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLP))
				eqpk -> LP = CLP;
			if (right_turn(&(eqpj -> E), &(eqpj -> RP), &(eqpk -> RP))) {
				project_point(&(eqpj ->E),   &(eqpk -> DC),
					      &(eqpj -> RP), &CRP);
				if (right_turn(&(eqpk -> E), &CRP, &(eqpk -> LP)))
					return FALSE;
				if (right_turn(&(eqpk -> E), &CRP, &(eqpk -> RP)))
					eqpk -> RP = CRP;
			}
		}
	}

	return TRUE;
}


/*
 * Bottleneck property test
 */

	static
	bool
bsd_test (

struct einfo * eip,   /* IN - global EFST info */
struct eqp_t * eqpi,  /* IN - first eq-point */
struct eqp_t * eqpj,  /* IN - second eq-point */
struct eqp_t * eqpk   /* IN - new eq-point */
)
{
	double bsbs, a, b, aa, sin1, sin2;
	struct point CLP, CRP, CLP1, CRP1, CLP2, CRP2, ai;

	eqpk -> BS = getBSD(eip, eqpi, eqpj);
	eqpk -> UB = eqpi -> UB + eqpj -> UB + eqpk -> BS;

	memset (&CLP, 0, sizeof (CLP));

	if (eqpi -> R EQ NULL) {
		rotate2(&(eqpi -> E), &(eqpk -> DC),
			eqpk -> BS/(2.0*eqpk -> DR), 2.0, &CLP);

		if (NOT test_and_save_LP(eip, eqpi, eqpj, eqpk, &CLP)) return FALSE;
	}
	else {
		bsbs = eqpk -> BS * eqpk -> BS;
		project_point(&(eqpi -> E),  &(eqpi -> DC),
			      &(eqpk -> LP), &ai);
		aa   = sqr_dist(&ai, &(eqpk -> LP));
		if (bsbs < 0.999 * aa) {
			a    = sqrt(aa);
			b    = (a + 2.0 * (  EDIST(&(eqpj -> E), &(eqpk -> LP))
				+ EDIST(&(eqpi -> R -> E), &ai))) / SQRT3;

			if (solve_quadratic(aa + b*b, eqpk -> BS * b, bsbs - aa, &sin1, &sin2)) {
				memset (&CLP1, 0, sizeof (CLP1));
				memset (&CLP2, 0, sizeof (CLP2));
				rotate2(&(eqpk -> LP), &(eqpk -> DC),
					-sin1, (b * sin1 + eqpk -> BS)/a, &CLP1);
				rotate2(&(eqpk -> LP), &(eqpk -> DC),
					-sin2, (b * sin2 + eqpk -> BS)/a, &CLP2);
				if (right_turn(&(eqpk -> E), &CLP2, &CLP1))
					CLP1 = CLP2;

				if (NOT test_and_save_LP(eip, eqpi, eqpj, eqpk, &CLP1)) return FALSE;
			}
		}
	}

	memset (&CRP, 0, sizeof (CRP));

	if (eqpj -> R EQ NULL) {
		rotate2(&(eqpj -> E), &(eqpk -> DC),
			-eqpk -> BS/(2.0*eqpk -> DR), 2.0, &CRP);

		if (NOT test_and_save_RP(eip, eqpi, eqpj, eqpk, &CRP)) return FALSE;
	}
	else {
		bsbs = eqpk -> BS * eqpk -> BS;
		project_point(&(eqpj -> E),  &(eqpj -> DC),
			      &(eqpk -> RP), &ai);
		aa   = sqr_dist(&ai, &(eqpk -> RP));
		if (bsbs < 0.999 * aa) {
			a    = sqrt(aa);
			b    = (a + 2.0 * (  EDIST(&(eqpi -> E), &(eqpk -> RP))
				+ EDIST(&(eqpj -> L -> E), &ai))) / SQRT3;

			if (solve_quadratic(aa + b*b, eqpk -> BS * b, bsbs - aa, &sin1, &sin2)) {
				memset (&CRP1, 0, sizeof (CRP1));
				memset (&CRP2, 0, sizeof (CRP2));
				rotate2(&(eqpk -> RP), &(eqpk -> DC),
					sin1, (b * sin1 + eqpk -> BS)/a, &CRP1);
				rotate2(&(eqpk -> RP), &(eqpk -> DC),
					sin2, (b * sin2 + eqpk -> BS)/a, &CRP2);
				if (right_turn(&(eqpk -> E), &CRP1, &CRP2))
					CRP1 = CRP2;

				if (NOT test_and_save_RP(eip, eqpi, eqpj, eqpk, &CRP1)) return FALSE;
			}
		}
	}

	return TRUE;
}


/*
 * Lune property test
 */

	static
	bool
lune_test (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpi, /* IN - first eq-point */
struct eqp_t * eqpj, /* IN - second eq-point */
struct eqp_t * eqpk  /* IN - new eq-point */
)
{
	int r;
	struct point CJ, AI, CLP, CLPP, CRP, CRPP;
	dist_t a, aa, b, bb, c, d, e, f, ff, g, gg, h, hh;
	dist_t dist_qicj, dist_oacr, dist_opqr, dist_pqi, dist_piai, dist_qpi;
	dist_t sin1, sin2;

	memset (&CLPP, 0, sizeof (CLPP));
	memset (&CRPP, 0, sizeof (CRPP));

	if (eqpi -> R) {
		project_point(&(eqpi -> E),  &(eqpi -> DC),
			      &(eqpk -> LP), &CJ);

		aa	  = sqr_dist(&CJ, &(eqpk -> LP));
		dist_qicj = 0.999 * aa; /* only look at terminals which really are inside lune */

		for (r = 0; r < eip -> pts -> n; r++) {
			if (sqr_dist(&(eip -> eqp[r].E), &CJ)		>= dist_qicj) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpk -> LP)) >= dist_qicj) continue;

			CLP = eqpi -> E;
			a   = sqrt(aa);
			b   = (a + 2.0 * (  EDIST(&(eqpj -> E), &(eqpk -> LP))
				 + EDIST(&(eqpi -> R -> E), &CJ))) / SQRT3;
			bb  = b*b;
			dist_oacr = EDIST(&(eip -> eqp[r].E), &(eqpi -> DC));
			c = dist_oacr*dist_oacr + eqpi -> DR2;
			d = 2.0*((CJ.x - eqpi -> DC.x) * (eqpi -> DC.x - eip -> eqp[r].E.x) +
				 (CJ.y - eqpi -> DC.y) * (eqpi -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((CJ.y - eqpi -> DC.y) * (eqpi -> DC.x - eip -> eqp[r].E.x) -
				 (CJ.x - eqpi -> DC.x) * (eqpi -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = -e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin1, (f+g*sin1)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin2, (f+g*sin2)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
			}

			dist_opqr = EDIST(&(eip -> eqp[r].E), &(eqpk -> DC));
			c = dist_opqr*dist_opqr + eqpk -> DR2;
			d = 2.0*((eqpk -> LP.x - eqpk -> DC.x) * (eqpk -> DC.x - eip -> eqp[r].E.x) +
				 (eqpk -> LP.y - eqpk -> DC.y) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((eqpk -> LP.y - eqpk -> DC.y) * (eqpk -> DC.x - eip -> eqp[r].E.x) -
				 (eqpk -> LP.x - eqpk -> DC.x) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = -e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin1, (f+g*sin1)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin2, (f+g*sin2)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
			}

			if (NOT test_and_save_LP(eip, eqpi, eqpj, eqpk, &CLP)) return FALSE;

			project_point(&(eqpi -> E),  &(eqpi -> DC),
				      &(eqpk -> LP), &CJ);
			aa	  = sqr_dist(&CJ, &(eqpk -> LP));
			dist_qicj = 0.999 * aa;
		}
	}
	else {
		aa	 = sqr_dist(&(eqpi -> E), &(eqpk -> LP));
		dist_pqi = 0.999 * aa;

		for (r = 0; r < eip -> pts -> n; r++) {
			if (r EQ eqpi -> E.pnum) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpi -> E))	>= dist_pqi) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpk -> LP)) >= dist_pqi) continue;

			rotate2(&(eqpi -> E), &(eqpk -> DC),
				EDIST(&(eip -> eqp[r].E), &(eqpi -> E))/(2.0*eqpk -> DR), 2.0, &CLP);

			a  = sqrt(aa);
			b  = (a + 2.0 * EDIST(&(eqpj -> E), &(eqpk -> LP))) / SQRT3;
			bb = b*b;
			dist_opqr = EDIST(&(eip -> eqp[r].E), &(eqpk -> DC));
			c = dist_opqr*dist_opqr + eqpk -> DR2;
			d = 2.0*((eqpk -> LP.x - eqpk -> DC.x) * (eqpk -> DC.x - eip -> eqp[r].E.x) +
				 (eqpk -> LP.y - eqpk -> DC.y) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((eqpk -> LP.y - eqpk -> DC.y) * (eqpk -> DC.x - eip -> eqp[r].E.x) -
				 (eqpk -> LP.x - eqpk -> DC.x) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = -e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin1, (f+g*sin1)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
				rotate(&(eqpk -> LP), &(eqpk -> DC), -sin2, (f+g*sin2)/h, &CLPP);
				if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
				    left_turn (&(eqpk -> E), &CLP, &CLPP))
					CLP = CLPP;
			}

			if (NOT test_and_save_LP(eip, eqpi, eqpj, eqpk, &CLP)) return FALSE;

			aa	 = sqr_dist(&(eqpi -> E), &(eqpk -> LP));
			dist_pqi = 0.999 * aa;
		}
	}


	if (eqpj -> R) {
		project_point(&(eqpj -> E),  &(eqpj -> DC),
			      &(eqpk -> RP), &AI);

		aa	  = sqr_dist(&AI, &(eqpk -> RP));
		dist_piai = 0.999 * aa;

		for (r = 0; r < eip -> pts -> n; r++) {
			if (sqr_dist(&(eip -> eqp[r].E), &AI)		>= dist_piai) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpk -> RP)) >= dist_piai) continue;

			CRP = eqpj -> E;
			a   = sqrt(aa);
			b   = (a + 2.0 * (  EDIST(&(eqpi -> E), &(eqpk -> RP))
				 + EDIST(&(eqpj -> L -> E), &AI))) / SQRT3;
			bb  = b*b;
			dist_oacr = EDIST(&(eip -> eqp[r].E), &(eqpj -> DC));
			c = dist_oacr*dist_oacr + eqpj -> DR2;
			d = 2.0*((AI.x - eqpj -> DC.x) * (eqpj -> DC.x - eip -> eqp[r].E.x) +
				 (AI.y - eqpj -> DC.y) * (eqpj -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((AI.y - eqpj -> DC.y) * (eqpj -> DC.x - eip -> eqp[r].E.x) -
				 (AI.x - eqpj -> DC.x) * (eqpj -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin1, (f+g*sin1)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin2, (f+g*sin2)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
			}

			dist_opqr = EDIST(&(eip -> eqp[r].E), &(eqpk -> DC));
			c = dist_opqr*dist_opqr + eqpk -> DR2;
			d = 2.0*((eqpk -> RP.x - eqpk -> DC.x) * (eqpk -> DC.x - eip -> eqp[r].E.x) +
				 (eqpk -> RP.y - eqpk -> DC.y) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((eqpk -> RP.y - eqpk -> DC.y) * (eqpk -> DC.x - eip -> eqp[r].E.x) -
				 (eqpk -> RP.x - eqpk -> DC.x) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin1, (f+g*sin1)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin2, (f+g*sin2)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
			}

			if (NOT test_and_save_RP(eip, eqpi, eqpj, eqpk, &CRP)) return FALSE;

			project_point(&(eqpj -> E),  &(eqpj -> DC),
				      &(eqpk -> RP), &AI);
			aa	  = sqr_dist(&AI, &(eqpk -> RP));
			dist_piai = 0.999 * aa;
		}
	}
	else {
		aa	 = sqr_dist(&(eqpj -> E), &(eqpk -> RP));
		dist_qpi = 0.999 * aa;

		for (r = 0; r < eip -> pts -> n; r++) {
			if (r EQ eqpj -> E.pnum) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpj -> E))	>= dist_qpi) continue;
			if (sqr_dist(&(eip -> eqp[r].E), &(eqpk -> RP)) >= dist_qpi) continue;

			rotate2(&(eqpj -> E), &(eqpk -> DC),
				-EDIST(&(eip -> eqp[r].E), &(eqpj -> E))/(2.0*eqpk -> DR), 2.0, &CRP);

			a  = sqrt(aa);
			b  = (a + 2.0 * EDIST(&(eqpi -> E), &(eqpk -> RP))) / SQRT3;
			bb = b*b;
			dist_opqr = EDIST(&(eip -> eqp[r].E), &(eqpk -> DC));

			c = dist_opqr*dist_opqr + eqpk -> DR2;
			d = 2.0*((eqpk -> RP.x - eqpk -> DC.x) * (eqpk -> DC.x - eip -> eqp[r].E.x) +
				 (eqpk -> RP.y - eqpk -> DC.y) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			e = 2.0*((eqpk -> RP.y - eqpk -> DC.y) * (eqpk -> DC.x - eip -> eqp[r].E.x) -
				 (eqpk -> RP.x - eqpk -> DC.x) * (eqpk -> DC.y - eip -> eqp[r].E.y));
			f = (aa+bb)/2.0-c; ff = f*f;
			g = e-a*b;	   gg = g*g;
			h = d-(aa-bb)/2.0; hh = h*h;
			if (solve_quadratic(gg + hh, f*g, ff - hh, &sin1, &sin2)) {
				if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) continue;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin1, (f+g*sin1)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
				rotate(&(eqpk -> RP), &(eqpk -> DC), sin2, (f+g*sin2)/h, &CRPP);
				if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
				    right_turn(&(eqpk -> E), &CRP, &CRPP))
					CRP = CRPP;
			}

			if (NOT test_and_save_RP(eip, eqpi, eqpj, eqpk, &CRP)) return FALSE;

			aa	 = sqr_dist(&(eqpj -> E), &(eqpk -> RP));
			dist_qpi = 0.999 * aa;
		}
	}

	return TRUE;
}


/*
 * Upper bound test
 */

	static
	bool
upper_bound_test (

struct einfo * eip,  /* IN - global EFST info */
struct eqp_t * eqpi, /* IN - first eq-point */
struct eqp_t * eqpj, /* IN - second eq-point */
struct eqp_t * eqpk  /* IN - new eq-point */
)
{
	dist_t	low_arc_length, rlb, llb, lower_bound, upper_bound, dist, distr, distl,
		rsmt, lsmt, dx, dy, l,
		ub_ri = INF_DISTANCE, ub_lj = INF_DISTANCE, ub_rk = INF_DISTANCE, ub_lk = INF_DISTANCE,
		distrj, distli, cosp, sinp, a, aa, b, bb, sin1, sin2;
	struct point P, CRP, CRPP, CLP, CLPP, M;

	/* Lower bound */

	if (EDIST(&(eqpk -> RP), &(eqpk -> LP)) < eip -> eps) return FALSE;
	rlb = EDIST(&(eqpk -> RP), &(eqpk -> E)) * (1.0 - eip -> eps * ((double) eqpk -> S));
	llb = EDIST(&(eqpk -> LP), &(eqpk -> E)) * (1.0 - eip -> eps * ((double) eqpk -> S));
	lower_bound = MIN(rlb, llb);

	M.x = (eqpk -> RP.x + eqpk -> LP.x) / 2.0;
	M.y = (eqpk -> RP.y + eqpk -> LP.y) / 2.0;
	P.x = eqpk -> DC.x + (eqpk -> DR / EDIST(&(eqpk -> DC), &M))* (M.x - eqpk -> DC.x);
	P.y = eqpk -> DC.y + (eqpk -> DR / EDIST(&(eqpk -> DC), &M))* (M.y - eqpk -> DC.y);
	low_arc_length = 2.0*EDIST(&P, &(eqpk -> RP));

	/* 1. upper bound */

	distr = closest_terminal(eip, &(eqpk -> RP), eqpi);
	distl = closest_terminal(eip, &(eqpk -> LP), eqpj);
	upper_bound = (eqpi -> UB + distr) + (eqpj -> UB + distl) + low_arc_length;
	if (upper_bound	 < lower_bound) return FALSE;
	rsmt = upper_bound;
	lsmt = upper_bound;

	/* 2. upper bound */

	dist = distl;
	if (distr < dist) dist = distr;
	upper_bound = eqpi -> UB + eqpj -> UB + dist +
		      EDIST(&(eqpk -> RP), &(eqpk -> LP)) + eqpk -> BS;
	if (upper_bound	 < lower_bound) return FALSE;
	if (rsmt > upper_bound) rsmt = upper_bound;
	if (lsmt > upper_bound) lsmt = upper_bound;

	/* 3. upper bound */

	if (eqpi -> R EQ NULL)
		ub_ri = EDIST(&(eqpk -> RP), &(eqpi -> E));
	else {
		eip -> termlist -> a[0] = eqpk -> RP;
		eip -> termlist -> n	= 1;
		eqpoint_terminals(eip, eqpi, eip -> termlist, TRUE);
		ub_ri = upper_bound_heuristic(eip -> termlist, eip -> bsd);
	}
	upper_bound = ub_ri + (eqpj -> UB + distl) + low_arc_length;
	if (upper_bound < lower_bound) return FALSE;
	if (rsmt > upper_bound) rsmt = upper_bound;
	if (lsmt > upper_bound) lsmt = upper_bound;

	/* 4. upper bound */

	if (eqpj -> R EQ NULL)
		ub_lj = EDIST(&(eqpk -> LP), &(eqpj -> E));
	else {
		eip -> termlist -> a[0] = eqpk -> LP;
		eip -> termlist -> n	= 1;
		eqpoint_terminals(eip, eqpj, eip -> termlist, TRUE);
		ub_lj = upper_bound_heuristic(eip -> termlist, eip -> bsd);
	}
	upper_bound = eqpi -> UB + distr;
	if (ub_ri < upper_bound) upper_bound = ub_ri;
	upper_bound += ub_lj + low_arc_length;
	if (upper_bound < lower_bound) return FALSE;
	if (rsmt > upper_bound) rsmt = upper_bound;
	if (lsmt > upper_bound) lsmt = upper_bound;

	/* 5. upper bound */

	upper_bound = 0.0;
	eip -> termlist -> a[0] = eqpk -> RP;
	eip -> termlist -> a[1] = eqpk -> LP;
	eip -> termlist -> n	= 2;
	if (eqpi -> R EQ NULL)
		upper_bound += distr;
	else
		eqpoint_terminals(eip, eqpi, eip -> termlist, TRUE);
	if (eqpj -> R EQ NULL)
		upper_bound += distl;
	else
		eqpoint_terminals(eip, eqpj, eip -> termlist, TRUE);
	if (eip -> termlist -> n EQ 2)
		upper_bound += eqpk -> BS + low_arc_length/2.0;
		/* was arc_length before (changed 12dec98) */
	else
		upper_bound += upper_bound_heuristic(eip -> termlist, eip -> bsd) +
			       low_arc_length/2.0;
	if (upper_bound < lower_bound) return FALSE;
	if (rsmt > upper_bound) rsmt = upper_bound;
	if (lsmt > upper_bound) lsmt = upper_bound;

	/* 6. upper bound */

	if (eqpi -> R) {
		eip -> termlist -> a[0] = eqpk -> RP;
		eip -> termlist -> n	= 1;
		eqpoint_terminals(eip, eqpk, eip -> termlist, TRUE);
		ub_rk = upper_bound_heuristic(eip -> termlist, eip -> bsd);
		upper_bound = ub_rk + low_arc_length;
		if (upper_bound < lower_bound) return FALSE;
		if (rsmt > upper_bound) rsmt = upper_bound;
	}

	/* 7. upper bound */

	if (eqpj -> R) {
		eip -> termlist -> a[0] = eqpk -> LP;
		eip -> termlist -> n	= 1;
		eqpoint_terminals(eip, eqpk, eip -> termlist, TRUE);
		ub_lk = upper_bound_heuristic(eip -> termlist, eip -> bsd);
		upper_bound = ub_lk + low_arc_length;
		if (upper_bound < lower_bound) return FALSE;
		if (lsmt > upper_bound) lsmt = upper_bound;
	}

	/* 8. upper bound */

	upper_bound = eqpj -> UB + low_arc_length + eqpk -> BS;
	if (eqpi -> R EQ NULL)
		upper_bound += EDIST(&(eqpk -> RP), &(eqpi -> E));
	else
		upper_bound += ub_ri;
	if (upper_bound < lower_bound) return FALSE;
	if (rsmt > upper_bound) rsmt = upper_bound;

	/* 9. upper bound */

	upper_bound = eqpi -> UB + low_arc_length + eqpk -> BS;
	if (eqpj -> R EQ NULL)
		upper_bound += EDIST(&(eqpk -> LP), &(eqpj -> E));
	else
		upper_bound += ub_lj;
	if (upper_bound < lower_bound) return FALSE;
	if (lsmt > upper_bound) lsmt = upper_bound;

	/* Do the final push */

	distrj = closest_terminal(eip, &(eqpk -> RP), eqpj);
	upper_bound = eqpi -> UB + eqpj -> UB + distr + distrj;
	if (rsmt > upper_bound) rsmt = upper_bound;
	distli = closest_terminal(eip, &(eqpk -> LP), eqpi);
	upper_bound = eqpi -> UB + eqpj -> UB + distl + distli;
	if (lsmt > upper_bound) lsmt = upper_bound;

	if (eqpi -> R NE NULL)
		upper_bound = ub_rk;
	else {
		if (eqpj -> R EQ NULL)
			upper_bound = EDIST(&(eqpj -> E), &(eqpk -> RP)) +
				      EDIST(&(eqpi -> E), &(eqpk -> RP));
		else {
			eip -> termlist -> a[0] = eqpk -> RP;
			eip -> termlist -> n	= 1;
			eqpoint_terminals(eip, eqpj, eip -> termlist, TRUE);
			upper_bound = upper_bound_heuristic(eip -> termlist, eip -> bsd) +
					EDIST(&(eqpi -> E), &(eqpk -> RP));
		}
	}
	if (upper_bound < rsmt) rsmt = upper_bound;

	if (rsmt < 0.999 * rlb) {
		CRP = eqpj -> E;
		memset (&CRPP, 0, sizeof (CRPP));
		get_angle_vector(&(eqpi -> E), &(eqpk -> E), &(eqpk -> RP), &dx, &dy);
		l = sqrt(dx*dx + dy*dy);
		cosp = dx/l; sinp = dy/l;
		a = eqpk -> DR*(SQRT3*cosp+sinp);     aa = a*a;
		b = eqpk -> DR*(SQRT3*sinp-cosp+2.0); bb = b*b;

		if (solve_quadratic(aa + bb, rsmt*b, rsmt*rsmt - aa, &sin1, &sin2)) {
			if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) return TRUE;
			rotate(&(eqpk -> RP), &(eqpk -> DC), sin1, (b*sin1+rsmt)/a, &CRPP);
			if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
			    right_turn(&(eqpk -> E), &CRP, &CRPP))
				CRP = CRPP;
			rotate(&(eqpk -> RP), &(eqpk -> DC), sin2, (b*sin2+rsmt)/a, &CRPP);
			if (left_turn(&(eqpk -> E), &(eqpk -> RP), &CRPP) AND
			    right_turn(&(eqpk -> E), &CRP, &CRPP))
				CRP = CRPP;

			if (NOT test_and_save_RP(eip, eqpi, eqpj, eqpk, &CRP)) return FALSE;
		}
	}

	if (eqpj -> R)
		upper_bound = ub_lk;
	else {
		if (eqpi -> R EQ NULL)
			upper_bound = EDIST(&(eqpi -> E), &(eqpk -> LP)) +
				      EDIST(&(eqpj -> E), &(eqpk -> LP));
		else {
			eip -> termlist -> a[0] = eqpk -> LP;
			eip -> termlist -> n	= 1;
			eqpoint_terminals(eip, eqpi, eip -> termlist, TRUE);
			upper_bound = upper_bound_heuristic(eip -> termlist, eip -> bsd) +
				      EDIST(&(eqpj -> E), &(eqpk -> LP));
		}
	}
	if (upper_bound < lsmt) lsmt = upper_bound;

	if (lsmt < 0.999 * llb) {
		CLP = eqpi -> E;
		memset (&CLPP, 0, sizeof (CLPP));
		get_angle_vector(&(eqpk -> LP), &(eqpk -> E), &(eqpj -> E), &dx, &dy);
		l = sqrt(dx*dx + dy*dy);
		cosp = dx/l; sinp = dy/l;
		a = eqpk -> DR*(SQRT3*cosp+sinp);     aa=a*a;
		b = eqpk -> DR*(SQRT3*sinp-cosp+2.0); bb=b*b;

		if (solve_quadratic(aa + bb, lsmt*b, lsmt*lsmt - aa, &sin1, &sin2)) {
			if ((fabs(sin1) < eip -> eps) OR (fabs(sin2) < eip -> eps)) return TRUE;
			rotate(&(eqpk -> LP), &(eqpk -> DC), -sin1, (b*sin1+lsmt)/a, &CLPP);
			if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
			    left_turn(&(eqpk -> E), &CLP, &CLPP))
				CLP = CLPP;
			rotate(&(eqpk -> LP), &(eqpk -> DC), -sin2, (b*sin2+lsmt)/a, &CLPP);
			if (right_turn(&(eqpk -> E), &(eqpk -> LP), &CLPP) AND
			    left_turn(&(eqpk -> E), &CLP, &CLPP))
				CLP = CLPP;

			if (NOT test_and_save_LP(eip, eqpi, eqpj, eqpk, &CLP)) return FALSE;
		}
	}

	return TRUE;
}


/*
 * Wedge property test
 */

	static
	bool
wedge_test (

struct einfo * eip,   /* IN - global EFST info */
struct eqp_t * eqpi,  /* IN - first eq-point */
struct eqp_t * eqpj,  /* IN - second eq-point */
struct eqp_t * eqpk   /* IN - new eq-point */
)
{
	int r, t;
	int right_counter = 0;
	int middle_counter = 0;
	int left_counter = 0;
	int top = highest_terminal(eqpk);
	bool flag;
	dist_t dist, dist1, dist2, bsdi, bsdj;
	struct point SP;
	struct eqp_t * eqpt;
	struct eqp_t * other_eqp;

	other_eqp = eqpj;
	if (NOT disjoint(eip, other_eqp)) other_eqp = eqpi;
	set_member_arr(eip, other_eqp, TRUE);

#ifdef HAVE_GMP
	if (Multiple_Precision > 0) {
		update_eqpoint_and_displacement (eip, eqpk);
	}
#endif

	for (t = 0; t < eip -> pts -> n; t++) {

		/* Terminal should not be part of this eq-point */
		if (eip -> MEMB[t]) continue;

		eqpt = &(eip -> eqp[t]);

		/* Is terminal completely outside feasible area? */
		if ((left_turn(&(eqpi -> E), &(eqpk -> LP), &(eqpt -> E))) OR
		    (right_turn(&(eqpj -> E), &(eqpk -> RP), &(eqpt ->E)))) continue;

		if (left_turn(&(eqpk -> E), &(eqpk -> LP), &(eqpt -> E))) {
			left_counter++; continue;
		}

		if (right_turn(&(eqpk -> E), &(eqpk -> RP), &(eqpt -> E))) {
			right_counter++; continue;
		}

		if (sqr_dist(&(eqpk -> DC), &(eqpt -> E)) > eqpk -> DR2) {
			middle_counter++;
			if (t <= top) continue; /* avoid duplicates */

			project_point(&(eqpk -> E), &(eqpk -> DC),
				      &(eqpt -> E), &SP);
			dist = EDIST(&(eqpt -> E), &SP) *
			       (1.0 - eip -> eps * ((double) eqpk -> S));

			/* Is the last edge too long? */
			if (dist >= getBSD(eip, eqpt, eqpk)) continue;

			flag = TRUE;
			for (r = 0; r < eip -> pts -> n; r++) {
				if (r EQ t) continue;
				if ((EDIST(&SP, &(eip -> eqp[r].E))	     < dist) AND
				    (EDIST(&(eqpt -> E), &(eip -> eqp[r].E)) < dist)) {
					flag = FALSE; break;
				}
			}
			if (NOT flag) continue;

			eip -> termlist -> a[0] = eqpt -> E;
			eip -> termlist -> n	= 1;
			eqpoint_terminals(eip, eqpk, eip -> termlist, TRUE);

			if (eqpk -> S > 2) {
				dist1 = upper_bound_heuristic(eip -> termlist, eip -> bsd);
				dist2 = eq_point_dist(eip, eqpt, eqpk) *
					(1.0 - eip -> eps * ((double) eqpk -> S));
				if (dist1 < dist2) {
					flag = FALSE;
				}
				else {
					bsdi = getBSD(eip, eqpi, eqpt);
					bsdj = getBSD(eip, eqpj, eqpt);
					if ((bsdi + eqpk -> UB < dist2) OR
					    (bsdj + eqpk -> UB < dist2) OR
					    (bsdi + bsdj + eqpi -> UB + eqpj -> UB < dist2))
						flag = FALSE;
				}
			}
			if (NOT flag) continue;

			/* This FST survived all tests. Save it! */
			test_and_save_fst(eip, eqpt, eqpk);
		}
	}

	set_member_arr(eip, other_eqp, FALSE);
	flag = FALSE;
	if (middle_counter >= 1) {
		flag = TRUE;
		if ((middle_counter EQ 1) AND (left_counter + right_counter EQ 0)) return FALSE;
	}
	else {
		if ((left_counter >= 1) AND (right_counter >= 1)) flag = TRUE;
	}
	return(flag);
}

/*
 * Compute the EFSTs for the given set of terminals, which are now
 * guaranteed to be unique.
 */

	static
	void
compute_efsts_for_unique_terminals (

struct einfo *		eip	/* IN - global EFST info */
)
{
int			n;
int			nedges;
struct pset *		pts;
struct edge *		ep;
struct edge *		mst_edges;
dist_t			mst_len;
char			buf1 [32];
eterm_t			*new_Zp;
int			i, j, k, si, l, size, iter, sz, starti, endi;
dist_t			upper_bound;
struct eqp_t		*eqpi, *eqpj, *eqpk, *eqpt, *eqp_old;
struct eqp_t		**eqp_list, **eqpp, **eqppp;
struct elist		*rp;

	pts = eip -> pts;
	n = pts -> n;

	/* Compute minimum spanning tree */

	mst_edges = NEWA (n - 1, struct edge);
	nedges = euclidean_mst (pts, mst_edges);
	if (nedges NE n - 1) {
		fatal ("compute_efsts_for_unique_terminals: Bug 1.");
	}

	mst_len = 0.0;
	eip -> max_mst_edge = 0.0;
	ep = mst_edges;
	for (i = 0; i < nedges; ep++, i++) {
		mst_len += ep -> len;
		/* Get longest MST edge */
		if (eip -> max_mst_edge < ep -> len) eip -> max_mst_edge = ep -> len;
	}
	eip -> mst_length = mst_len;

	if (Print_Detailed_Timings) {
		convert_delta_cpu_time (buf1);
		fprintf (stderr, "Compute MST:            %s\n", buf1);
	}

	/* Set relative epsilon for floating point comparisons.
	   We use the constant EpsilonFactor to indicate
	   the maximum relative error that is expected.
	*/

	eip -> eps   = ((double) EpsilonFactor) * DBL_EPSILON;

	/* Compute bottleneck Steiner distances */

	eip -> bsd = compute_bsd (nedges, mst_edges, 0);
	free ((char *) mst_edges);

	if (Print_Detailed_Timings) {
		convert_delta_cpu_time (buf1);
		fprintf (stderr, "Compute BSD:            %s\n", buf1);
	}

	/* Set up hashing data structure */

	eip -> term_check	= NEWA (n, bool);
	eip -> hash		= NEWA (n, struct elist *);

	for (i = 0; i < n; i++) {
	  eip -> term_check [i] = FALSE;
	  eip -> hash [i] = NULL;
	}
	eip -> list.forw	= &(eip -> list);
	eip -> list.back	= &(eip -> list);

	/* Compute the mean of all terminals.  We translate the terminals */
	/* so that the mean is at the origin.  The coordinates of the */
	/* translated instance have smaller magnitude, which permits us */
	/* to compute eq-point coordinates with higher precision. */

	eip -> mean.x = 0.0;
	eip -> mean.y = 0.0;
	for (k = 0; k < n; k++) {
		eip -> mean.x += pts -> a[k].x;
		eip -> mean.y += pts -> a[k].y;
	}
	eip -> mean.x = floor( eip -> mean.x / (double) n);
	eip -> mean.y = floor( eip -> mean.y / (double) n);

	/* Initialize array of equilateral points */

	eip -> eqp_size		= InitialEqPointsTerminal * n;
	eip -> eqp		= NEWA (eip -> eqp_size, struct eqp_t);
	eip -> size_start	= NEWA (n, int);
	eip -> eqpZ_size	= 10 * eip -> eqp_size;
	eip -> eqpZ		= NEWA (eip -> eqpZ_size, eterm_t);
	eip -> eqpZ_curr	= eip -> eqpZ;
	eip -> MEMB		= NEWA (n, bool);
	initialize_eqp_rectangles(eip);
	eip -> fsts_checked = 0;

#ifdef HAVE_GMP
	if (Multiple_Precision > 0) {
		qr3_init (&(eip -> cur_eqp.x));
		qr3_init (&(eip -> cur_eqp.y));
	}
#endif

	for (k = 0; k < n; k++) {
		eqpk = &(eip -> eqp[k]);
		memset (&(eqpk -> E), 0, sizeof (eqpk -> E));
		memset (&(eqpk -> DV), 0, sizeof (eqpk -> DV));
		eqpk -> E.x	= pts -> a[k].x - eip -> mean.x; /* translate terminals */
		eqpk -> E.y	= pts -> a[k].y - eip -> mean.y;
		eqpk -> E.pnum	= pts -> a[k].pnum;
		eqpk -> DV.x	= 0.0;
		eqpk -> DV.y	= 0.0;
		eqpk -> DV.pnum = k;
		eqpk -> R	= NULL;
		eqpk -> L	= NULL;
		eqpk -> S	= 1;
		eqpk -> UB	= 0.0;
		eqpk -> CHOSEN	= FALSE;
		eqpk -> Z	= eip -> eqpZ_curr++;
		*(eqpk -> Z)	= k;
		eip -> MEMB[k]	= FALSE;
	}
	save_eqp_rectangles(eip, 0, n-2); /* skip last terminal */
	eip -> size_start[1] = 0;

	/* Main loop of equilateral point generation */

	k = n;
	eqpk = &(eip -> eqp[k]);
	eip -> termlist = NEW_PSET(n+2);
	eqp_list = NEWA( eip -> eqp_size, struct eqp_t *);
	if (MaxFSTSize EQ 0) MaxFSTSize = n;

	for (size = 2; size <= MaxFSTSize-1; size++) {
		starti = eip -> size_start[(size-1)/2 + 1];
		endi   = k;
		if (size EQ 2) endi = n-1; /* skip last terminal */
		eip -> size_start[size] = k;

		if (Print_Detailed_Timings) {
			fprintf (stderr,
				 "- starting eq-point size %d"
				 " (total number eq-points is %d)\n",
				 size, k);
		}

		for (i = starti; i < endi; i++) {
			eqpi = &(eip -> eqp[i]);
			set_member_arr(eip, eqpi, TRUE);
			generate_compatible_eqp(eip, size - eqpi -> S, eqpi, eqp_list);

			eqpp = eqp_list;
			while (*eqpp) {
				eqpj = *(eqpp++);
				eqpj -> CHOSEN = FALSE;
				j = eqpj -> E.pnum;
				if (j > i)				continue;
				if (NOT disjoint(eip,eqpj))		continue;
				for (iter = 1; iter <= 3; iter++) {
					if (iter >= 2) {
						struct eqp_t * eqptmp = eqpi;
						eqpi = eqpj; eqpj = eqptmp;   /* swap i and j */
						if (iter >= 3) break;	      /* finished */
					}

					if (NOT projection_test_case_I(eip, eqpi, eqpj)) continue;

					/* Compute new eq-point. We do this by first computing */
					/* its displacement relative to one of its terminals   */
					/* and then add the result to that point	       */

					eq_point_disp_vector(eip, eqpi, eqpj, eqpk);
					eqpk -> E = eip -> eqp [ eqpk -> DV.pnum ].E;
					eqpk -> E.x += eqpk -> DV.x;
					eqpk -> E.y += eqpk -> DV.y;
					eqpk -> E.pnum = k;

					eqpk -> R  = eqpi;
					eqpk -> L  = eqpj;
					eqpk -> S  = eqpi -> S + eqpj -> S;
					eqpk -> RP = eqpi -> E; eqpk -> RP.pnum = -1;
					eqpk -> LP = eqpj -> E; eqpk -> LP.pnum = -1;
					eqpk -> CHOSEN = FALSE;
					eq_circle_center(&(eqpi -> E), &(eqpj  -> E), &(eqpk -> E), &(eqpk -> DC));
					eqpk -> DR2 = sqr_dist(&(eqpk -> DC), &(eqpi -> E));

					if (NOT projection_test_cases_II_VI(eip, eqpi, eqpj, eqpk)) continue;

					eqpk -> DR = sqrt(eqpk -> DR2);
					if (NOT bsd_test(eip, eqpi, eqpj, eqpk))			 continue;
					if (NOT lune_test(eip, eqpi, eqpj, eqpk))			 continue;

					new_Zp = merge_terminal_lists(eip, eqpi, eqpj, eqpk);

					if (NOT upper_bound_test(eip, eqpi, eqpj, eqpk))		 continue;
					if (NOT wedge_test(eip, eqpi, eqpj, eqpk))			 continue;

					eip -> eqpZ_curr = new_Zp;

					if (eqpk -> S > 2) {
						eqpoint_terminals(eip, eqpk, eip -> termlist, FALSE);
						upper_bound = upper_bound_heuristic(eip -> termlist, eip -> bsd);
						if (eqpk -> UB > upper_bound) eqpk -> UB = upper_bound;
					}
					k++;
					if (k >= eip -> eqp_size) {
						/* Eq-point space exhausted - double array */

						if (Print_Detailed_Timings)
							fprintf(stderr, "- doubling eq-point array\n");
						eqp_old = eip -> eqp;
						eip -> eqp = NEWA ( eip -> eqp_size * 2, struct eqp_t );
						memcpy ( eip -> eqp, eqp_old, eip -> eqp_size * sizeof(struct eqp_t) );

						/* Update local pointers */
						eqpi = UPDATE_PTR( eqpi, eqp_old, eip -> eqp );
						eqpj = UPDATE_PTR( eqpj, eqp_old, eip -> eqp );
						eqpk = UPDATE_PTR( eqpk, eqp_old, eip -> eqp );

						/* Update and double eq-point list */
						eqppp = eqp_list;
						while (*eqppp) {
							*eqppp = UPDATE_PTR( *eqppp, eqp_old, eip -> eqp ); eqppp++;
						}
						eqppp = eqp_list;
						eqp_list = NEWA( eip -> eqp_size * 2, struct eqp_t *);
						memcpy ( eqp_list, eqppp, eip -> eqp_size * sizeof(struct eqp_t *) );
						eqpp = UPDATE_PTR( eqpp, eqppp, eqp_list );
						free( eqppp );

						/* Update eq-point array left/right pointers */
						for (eqpt = &(eip -> eqp[n]); eqpt <= eqpk; eqpt++) {
							eqpt -> L = UPDATE_PTR( eqpt -> L, eqp_old, eip -> eqp );
							eqpt -> R = UPDATE_PTR( eqpt -> R, eqp_old, eip -> eqp );
						}

						/* Update rectangle pointers */
						for (sz = 1; sz < size; sz++)
						 if (eip -> eqp_squares[sz] NE NULL)
						  for (si = 0; si < eip -> srangex * eip -> srangey; si++)
						   for (l = 0; l < eip -> eqp_squares[sz][si].n; l++)
						    eip -> eqp_squares[sz][si].eqp[l] =
						       UPDATE_PTR( eip -> eqp_squares[sz][si].eqp[l], eqp_old, eip -> eqp );
						free( eqp_old );
						eip -> eqp_size = 2 * eip -> eqp_size;
					}
					eqpk++;
				}
			}
			set_member_arr(eip, eqpi, FALSE);
		}
		save_eqp_rectangles(eip, eip -> size_start[size], k-1);
	}

	if (Print_Detailed_Timings)
		fprintf(stderr, "%d eq-points generated.\n", k);

	/* Finally add MST-edges */

	ep = &(eip -> bsd -> mst_edges [1]);
	for (i = 1; i < n; i++) {
#ifdef HAVE_GMP
		if (Multiple_Precision > 0) {
			update_eqpoint_and_displacement (eip,
							 &(eip -> eqp [ep -> p2]));
		}
#endif
		eip -> termlist -> a[0] = pts -> a[ ep -> p1 ];
		eip -> termlist -> a[1] = pts -> a[ ep -> p2 ];
		eip -> termlist -> n	= 2;
		test_and_save_fst (eip,
				   &(eip -> eqp[ ep -> p1 ]),
				   &(eip -> eqp[ ep -> p2 ]));
		++ep;
	}

	if (Print_Detailed_Timings) {
		convert_delta_cpu_time (buf1);
		fprintf (stderr, "Generating eq-points:   %s\n", buf1);
	}

	/* Clean up */

#ifdef HAVE_GMP
	if (Multiple_Precision > 0) {
		qr3_clear (&(eip -> cur_eqp.y));
		qr3_clear (&(eip -> cur_eqp.x));
	}
#endif

	free( eqp_list );
	free( eip -> termlist );

	destroy_eqp_rectangles(eip);

	free( eip -> MEMB );
	free( eip -> eqpZ );
	free( eip -> size_start );
	free( eip -> eqp );

	/* Disconnect FSTs from hash table. */
	for (rp = eip -> list.forw;
	     rp NE &(eip -> list);
	     rp = rp -> forw) {
		rp -> next = NULL;
	}

	free ( eip -> hash );
	free ( eip -> term_check );

	shutdown_bsd (eip -> bsd);
}

/*
 * This routine performs all of the FST specific screening tests.
 * If all are passed, the FST is saved.
 */

	static
	dist_t
test_and_save_fst (

struct einfo *	eip,		/* IN/OUT - The global EFST info */
struct eqp_t *	eqpt,		/* IN - terminal endpoint of this FST */
struct eqp_t *	eqpk		/* IN - eq-point of this FST */
)
{
int			i, j, k;
int			nedges;
int size, spidx, termidx;
dist_t length;
struct edge *		ep;
struct point *		sp;
struct point		nsp;
struct point *		tp;
struct pset *		pts;
struct point *		p1;
struct elist *		rp;
struct elist **		hookp;
struct elist *		rp1;
struct elist *		rp2;
struct pset *		new_terms;
struct pset *		new_steiners;
struct full_set *	fsp;
struct edge *		edges;

	/* Assume that termlist has been constructed (change later!!) */

	++(eip -> fsts_checked);

	pts	= eip -> pts;
	size	= eip -> termlist -> n;

#ifdef HAVE_GMP
	if (Multiple_Precision > 0) {
		/* Exact position of eqpk is already in eip -> cur_eqp. */
		length	= compute_EFST_length (eip, eqpt);
	}
	else {
		length	= eq_point_dist (eip, eqpt, eqpk);
	}
#else
	length	= eq_point_dist (eip, eqpt, eqpk);
#endif

	/* General duplicate test.  We use a hash table, for speed.	*/
	/* For correctness, the hash function must not depend upon the	*/
	/* order of the terminals in the FST.  A simple checksum has	*/
	/* this property and tends to avoid favoring any one bucket.	*/

	/* Compute hash and prepare for rapid set comparison. */
	k = 0;
	for (i = 0; i < size; i++) {
		j = eip -> termlist -> a[i].pnum;
		eip -> term_check [j] = TRUE;
		k += j;
	}
	k %= pts -> n;

	hookp = &(eip -> hash [k]);
	for (;;) {
		rp = *hookp;
		if (rp EQ NULL) break;
		if (rp -> size < size) { rp = NULL; break; } /* rest is smaller */
		if (rp -> size EQ size) {
			p1 = &(rp -> fst -> terminals -> a [0]);
			for (i = 0; ; i++, p1++) {
				if (i >= size) goto found_efst;
				if (NOT eip -> term_check [p1 -> pnum]) break;
			}
		}
		hookp = &(rp -> next);
	}

found_efst:

	for (i = 0; i < size; i++) {
		eip -> term_check [eip -> termlist ->a[i].pnum ] = FALSE;
	}

	if (rp NE NULL) {
		/* An FST for these terminals already exists. */
		fsp = rp -> fst;
		if (fsp -> tree_len <= length) {
			return (fsp -> tree_len);
		}
		/* The new one is shorter!  Delete the old one. */
		*hookp = rp -> next;
		rp2 = rp -> forw;
		rp1 = rp -> back;
		rp2 -> back = rp1;
		rp1 -> forw = rp2;
		free ((char *) (fsp -> terminals));
		free ((char *) (fsp -> steiners));
		free ((char *) (fsp -> edges));
		free ((char *) fsp);
		free ((char *) rp);
	}

	/* Build FST graph in edge list form. */

	new_terms = NEW_PSET (size);
	tp = &(new_terms -> a [0]);
	new_terms -> n = size;

	if (size <= 2) {
		new_steiners = NULL;
		nedges = 1;
		edges = NEWA (1, struct edge);
		tp [0] = eip -> termlist -> a [0];
		tp [1] = eip -> termlist -> a [1];
		edges -> p1   = 0;
		edges -> p2   = 1;
		edges -> len  = length;
	}
	else {
		new_steiners = NEW_PSET (size - 2);
		new_steiners -> n = size - 2;
		nedges = 2 * size - 3;
		edges = NEWA (nedges, struct edge);

		tp [0].x    = eip -> termlist -> a [0].x + eip -> mean.x;
		tp [0].y    = eip -> termlist -> a [0].y + eip -> mean.y;
		tp [0].pnum = eip -> termlist -> a [0].pnum;
		sp	    = new_steiners -> a;
		ep	    = edges;
		spidx	    = size;
		termidx	    = 0;
		project_point(&(eqpk -> E), &(eqpk -> DC), &(eqpt -> E), &nsp);
		nsp.pnum    = spidx++;
		sp -> x	    = nsp.x + eip -> mean.x; /* translate back */
		sp -> y	    = nsp.y + eip -> mean.y;
		sp -> pnum  = nsp.pnum;
		ep -> p1    = termidx++;
		ep -> p2    = nsp.pnum;
		ep -> len   = EDIST(&(eqpt -> E), &nsp);
		sp++;
		ep++;

		build_efst_graph (eip, &nsp, eqpk -> R, &sp, &ep, &spidx, tp, &termidx);
		build_efst_graph (eip, &nsp, eqpk -> L, &sp, &ep, &spidx, tp, &termidx);
	}

	fsp = NEW (struct full_set);

	fsp -> next		= NULL;
	fsp -> tree_num		= 0;
	fsp -> tree_len		= length;
	fsp -> terminals	= new_terms;
	fsp -> steiners		= new_steiners;
	fsp -> nedges		= nedges;
	fsp -> edges		= edges;

	rp = NEW (struct elist);

	rp2 = &(eip -> list);
	rp1 = rp2 -> back;
	rp -> back	= rp1;
	rp -> forw	= rp2;
	rp -> next	= eip -> hash [k];
	rp -> size	= size;
	rp -> fst	= fsp;

	rp1 -> forw	= rp;
	rp2 -> back	= rp;
	eip -> hash [k] = rp;

	return (length);
}

/*
 * This routine constructs a graph of the EFST in edge list form.
 * This routine also fills in the proper Steiner points.
 */

	static
	void
build_efst_graph (

struct einfo *		eip,
struct point *		nsp,
struct eqp_t *		eqpk,
struct point **		sp,
struct edge **		ep,
int *			spidx,
struct point *		tp,
int *			termidx
)
{
int			idx;
int			pnum;
struct point		nnsp;

	if (eqpk -> R EQ NULL) {
		/* This is a terminal */
		pnum = eqpk - eip -> eqp;
		idx = (*termidx)++;
		tp [idx] = eip -> pts -> a [pnum];

		(*ep) -> p1  = nsp -> pnum;
		(*ep) -> p2  = idx;
		(*ep) -> len = EDIST (nsp, &(eqpk -> E));
		(*ep)++;
	}
	else {
		project_point (&(eqpk -> E), &(eqpk -> DC), nsp, &nnsp);
		idx = (*spidx)++;
		nnsp.pnum     = idx;
		(*sp) -> x    = nnsp.x + eip -> mean.x; /* translate back */
		(*sp) -> y    = nnsp.y + eip -> mean.y;
		(*sp) -> pnum = idx;
		(*ep) -> p1   = nsp  -> pnum;
		(*ep) -> p2   = idx;
		(*ep) -> len  = EDIST (nsp, &nnsp);
		(*sp)++;
		(*ep)++;

		build_efst_graph (eip, &nnsp, eqpk -> R, sp, ep, spidx, tp, termidx);
		build_efst_graph (eip, &nnsp, eqpk -> L, sp, ep, spidx, tp, termidx);
	}
}

/*
 * Map all terminal numbers back to their original values.  (i.e., with
 * the duplicate terminals reinstated.	We must renumber the terminals
 * of each EFST also.
 */

	static
	void
renumber_terminals (

struct einfo *		eip,		/* IN/OUT - global EFST info */
struct pset *		to_pts,		/* IN - point set to map to (orig) */
int *			rev_map		/* IN - map from new to old terms */
)
{
int			i;
int			j;
int			from_n;
int			to_n;
int			kmasks;
struct pset *		from_pts;
struct elist *		rp1;
struct elist *		rp2;
struct full_set *	fsp;
struct pset *		terms;
struct point *		p1;

	from_pts	= eip -> pts;
	from_n		= from_pts -> n;
	to_n		= to_pts -> n;

	kmasks = eip -> num_term_masks;

	/* Restore original set of terminals. */
	eip -> pts = to_pts;

	/* Renumber terminals in each FST. */
	rp2 = &(eip -> list);
	for (rp1 = rp2 -> forw; rp1 NE rp2; rp1 = rp1 -> forw) {
		fsp = rp1 -> fst;
		terms = fsp -> terminals;
		p1 = &(terms -> a [0]);
		for (i = 0; i < terms -> n; i++, p1++) {
			j = p1 -> pnum;
			if ((j < 0) OR (j >= from_n)) {
				fatal ("renumber_terminals: Bug 1.");
			}
			j = rev_map [j];
			if ((j < 0) OR (j >= to_n)) {
				fatal ("renumber_terminals: Bug 2.");
			}
			p1 -> pnum = j;
		}
	}
}

/*
 * Link all of the FSTs together into one long list and number them
 * each sequentially.  Free the doubly-linked elists as we go.
 */

	static
	void
build_fst_list (

struct einfo *		eip		/* IN - global EFST info */
)
{
int			i;
struct elist *		rp1;
struct elist *		rp2;
struct elist *		rp3;
struct full_set *	fsp;
struct full_set **	hookp;

	hookp = &(eip -> full_sets);

	rp2 = &(eip -> list);
	i = 0;
	for (rp1 = rp2 -> forw; rp1 NE rp2; ) {
		fsp = rp1 -> fst;
		fsp -> tree_num = i++;
		*hookp = fsp;
		hookp = &(fsp -> next);
		rp3 = rp1;
		rp1 = rp1 -> forw;
		free ((char *) rp3);
	}
	*hookp = NULL;

	eip -> list.forw = rp2;
	eip -> list.back = rp2;

	/* Make it easy to add zero-length FSTs onto the end. */
	eip -> ntrees	= i;
	eip -> hookp	= hookp;
}

/*
 * This routine adds one EFST (of zero length) connecting the first
 * terminal of each duplicate terminal group to each terminal that
 * was deleted during the major processing.
 */

	static
	void
add_zero_length_fsts (

struct einfo *		eip,		/* IN - global EFST info */
int			ndg,		/* IN - number of duplicate groups */
int **			list		/* IN - list of duplicate groups */
)
{
int			i;
int			t;
int			u;
int			kmasks;
int *			ip1;
int *			ip2;
struct pset *		pts;
struct pset *		terms;
struct edge *		edges;
struct full_set *	fsp;

	pts	= eip -> pts;
	kmasks	= eip -> num_term_masks;

	for (i = 0; i < ndg; i++) {
		ip1 = list [i];
		ip2 = list [i + 1];
		if (ip1 + 2 > ip2) {
			/* Group with fewer than two members! */
			fatal ("add_zero_length_fsts: Bug 1.");
		}
		t = *ip1++;
		while (ip1 < ip2) {
			u = *ip1++;

			terms = NEW_PSET (2);
			terms -> n = 2;
			terms -> a [0]	= pts -> a [t];
			terms -> a [1]	= pts -> a [u];

			edges = NEW (struct edge);
			edges -> p1	= 0;
			edges -> p2	= 1;
			edges -> len	= 0.0;

			fsp = NEW (struct full_set);

			fsp -> next	 = NULL;
			fsp -> tree_num	 = (eip -> ntrees)++;
			fsp -> tree_len	 = 0.0;
			fsp -> terminals = terms;
			fsp -> steiners	 = NULL;
			fsp -> nedges	 = 1;
			fsp -> edges	 = edges;

			*(eip -> hookp) = fsp;
			eip -> hookp	= &(fsp -> next);
		}
	}
}

/*
 * This routine allocates an array of pointers that lets us access a particular
 * tree number directly.
 */

	static
	struct full_set **
put_trees_in_array (

struct full_set *	fsp,		/* IN - list of full-trees. */
int *			acount		/* OUT - count of array elements. */
)
{
struct full_set **	ap;
struct full_set *	p;
int			count;
int			num;
struct full_set **	array;

	count = 0;
	for (p = fsp; p NE NULL; p = p -> next) {
		++count;
	}

	array = NEWA (count, struct full_set *);

	num = 0;
	ap = array;
	for (p = fsp; p NE NULL; p = p -> next) {
		*ap++ = p;
	}

	*acount = count;

	return (array);
}

/*
 * Compute the CPU time used since the last time we called this routine.
 */

	static
	cpu_time_t
get_delta_cpu_time (void)

{
cpu_time_t		now;
cpu_time_t		delta;

	now = get_cpu_time ();

	delta = now - Tn;

	Tn = now;

	return (delta);
}


/*
 * Compute and format the CPU time used since the last time we called
 * this routine.
 */

	static
	void
convert_delta_cpu_time (

char *		buf		/* OUT - ASCII time string, CPU seconds */
)
{
cpu_time_t	delta;

	delta = get_delta_cpu_time ();
	convert_cpu_time (delta, buf);
}
