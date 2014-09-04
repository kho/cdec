/* standard headers */
#include <assert.h>
#include <zlib.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

using namespace std;

struct STreeItem{
	STreeItem(const char* pszTerm) {
		m_pszTerm = new char[ strlen( pszTerm ) + 1 ];
		strcpy( m_pszTerm, pszTerm );

		m_ptParent = NULL;
		m_iBegin = -1;
		m_iEnd = -1;
		m_iHeadIndex = -1;
		m_iBrotherIndex = -1;
		m_iDepHeadWordIndex = -1;
		m_iHeadWordIndex = -1;
	}
	~STreeItem( ) {
		delete [] m_pszTerm;
		for (size_t i = 0; i < m_vecChildren.size(); i++)
			delete m_vecChildren[i];
	}
	int fnAppend(STreeItem *ptChild) {
		m_vecChildren.push_back(ptChild);
		ptChild->m_iBrotherIndex = m_vecChildren.size() - 1;
		ptChild->m_ptParent = this;
		return m_vecChildren.size() - 1;
	}
	int fnGetChildrenNum() {
		return m_vecChildren.size();
	}

	bool fnIsPreTerminal( void ) {
		int I;
		if ( this == NULL || m_vecChildren.size() == 0 )
			return false;

		for ( I = 0; I < m_vecChildren.size(); I++ )
			if (m_vecChildren[I]->m_vecChildren.size() > 0 )
				return false;

		return true;
	}
public:
	char	*m_pszTerm;

	vector<STreeItem*> m_vecChildren;//children items
	STreeItem *m_ptParent;//the parent item

	int m_iBegin;
	int m_iEnd; //the node span words[m_iBegin, m_iEnd]
	int m_iHeadIndex; //the index of its head child
	int m_iBrotherIndex;//the index in his brothers

	/*
	 * the index of its head word (from the dependency view),
	 * here head is a word outside the node
	 */
	int m_iDepHeadWordIndex; //the index of its head word (from dependency view), here

	/*
	 * the index of its head word (from the constituency view),
	 * here head is a word within the node and can represent the node
	 */
	int m_iHeadWordIndex;
};

void fnReadSyntactic(const char *pszSyn, vector<string>& vec) {
	char *p;
	int I;

	int iLeftNum, iRightNum;
	char *pszTmp, *pszTerm;
	pszTmp = new char[strlen(pszSyn)];
	pszTerm = new char[strlen(pszSyn)];
	pszTmp[0] = pszTerm[0] = '\0';

	vec.clear();

	char *pszLine;
	pszLine = new char[strlen(pszSyn) + 1];
	strcpy( pszLine, pszSyn );

	char *pszLine2;

	while( 1 ) {
		while(( strlen( pszLine ) > 0 )
				&&( pszLine[ strlen( pszLine ) - 1 ] > 0 )
				&&( pszLine[ strlen( pszLine ) - 1 ] <= ' ' ) )
			pszLine[ strlen( pszLine ) - 1 ] = '\0';

		if( strlen( pszLine ) == 0 )
			break;

		//printf( "%s\n", pszLine );
		pszLine2 = pszLine;
		while( pszLine2[ 0 ] <= ' ' )
			pszLine2 ++;
		if( pszLine2[ 0 ] == '<' )
			continue;

		sscanf( pszLine2 + 1, "%s", pszTmp );

		if ( pszLine2[ 0 ] == '(' ) {
			iLeftNum = 0;
			iRightNum = 0;
		}

		p = pszLine2;
		while ( 1 ) {
			pszTerm[ 0 ] = '\0';
			sscanf( p, "%s", pszTerm );

			if( strlen( pszTerm ) == 0 )
				break;
			p = strstr( p, pszTerm );
			p += strlen( pszTerm );

			if( ( pszTerm[ 0 ] == '(' )
					||( pszTerm[ strlen( pszTerm ) - 1 ] == ')' ) ) {
				if( pszTerm[ 0 ] == '(' ) {
					vec.push_back(string("("));
					iLeftNum++;

					I = 1;
					while ( pszTerm[ I ] == '(' && pszTerm[ I ]!= '\0' ) {
						vec.push_back(string("("));
						iLeftNum++;

						I++;
					}

					if( strlen( pszTerm ) > 1 )
						vec.push_back(string(pszTerm + I));
				} else {
					char *pTmp;
					pTmp = pszTerm + strlen( pszTerm ) - 1;
					while( ( pTmp[ 0 ] == ')' ) && ( pTmp >= pszTerm ) )
						pTmp --;
					pTmp[ 1 ] = '\0';

					if( strlen( pszTerm ) > 0 )
						vec.push_back(string(pszTerm));
					pTmp += 2;

					for( I = 0; I <= (int)strlen( pTmp ); I ++ ) {
						vec.push_back(string(")"));
						iRightNum++;
					}
				}
			} else {
				char *q;
				q = strchr( pszTerm, ')' );
				if ( q != NULL ) {
					q[ 0 ] = '\0';
					if ( pszTerm[ 0 ] != '\0' )
						vec.push_back(string(pszTerm));
					vec.push_back(string(")"));
					iRightNum++;

					q++;
					while ( q[ 0 ] == ')' ) {
						vec.push_back(string(")"));
						q++;
						iRightNum ++;
					}

					while( q[ 0 ] == '(' ) {
						vec.push_back(string("("));
						q++;
						iLeftNum++;
					}

					if ( q[ 0 ] != '\0' )
						vec.push_back(string(q));
				}
				else
					vec.push_back(string(pszTerm));
			}
		}

		assert(iLeftNum == iRightNum);
		/*if ( iLeftNum != iRightNum ) {
			printf( "ERROR: left( and right ) is not matched, %d ( and %d )\n", iLeftNum, iRightNum );
			return;
		}*/


		if( vec.size() >= 2
				&& strcmp( vec[1].c_str(), "(" ) == 0 ) {
			//( (IP..) )
			std::vector<string>::iterator it;
			it = vec.begin();
			it++;
			vec.insert(it, string("ROOT"));
		}

		break;
	}

	delete [] pszLine;
	delete [] pszTmp;
	delete [] pszTerm;
}

struct SGetHeadWord{
	typedef vector<string> CVectorStr;
	SGetHeadWord() {

	}
	~SGetHeadWord() {

	}
	int fnGetHeadWord( char *pszCFGLeft, CVectorStr vectRight ) {
		//0 indicating from right to left while 1 indicating from left to right
		char szaHeadLists[ 201 ] = "0";

		/*  //head rules for Egnlish
		if( strcmp( pszCFGLeft, "ADJP" ) == 0 )
			strcpy( szaHeadLists, "0NNS 0QP 0NN 0$ 0ADVP 0JJ 0VBN 0VBG 0ADJP 0JJR 0NP 0JJS 0DT 0FW 0RBR 0RBS 0SBAR 0RB 0" );
		else if( strcmp( pszCFGLeft, "ADVP" ) == 0 )
			strcpy( szaHeadLists, "1RB 1RBR 1RBS 1FW 1ADVP 1TO 1CD 1JJR 1JJ 1IN 1NP 1JJS 1NN 1" );
		else if( strcmp( pszCFGLeft, "CONJP" ) == 0 )
			strcpy( szaHeadLists, "1CC 1RB 1IN 1" );
		else if( strcmp( pszCFGLeft, "FRAG" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "INTJ" ) == 0 )
			strcpy( szaHeadLists, "0" );
		else if( strcmp( pszCFGLeft, "LST" ) == 0 )
			strcpy( szaHeadLists, "1LS 1: 1CLN 1" );
		else if( strcmp( pszCFGLeft, "NAC" ) == 0 )
			strcpy( szaHeadLists, "0NN 0NNS 0NNP 0NNPS 0NP 0NAC 0EX 0$ 0CD 0QP 0PRP 0VBG 0JJ 0JJS 0JJR 0ADJP 0FW 0" );
		else if( strcmp( pszCFGLeft, "PP" ) == 0 )
			strcpy( szaHeadLists, "1IN 1TO 1VBG 1VBN 1RP 1FW 1" );
		else if( strcmp( pszCFGLeft, "PRN" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "PRT" ) == 0 )
			strcpy( szaHeadLists, "1RP 1" );
		else if( strcmp( pszCFGLeft, "QP" ) == 0 )
			strcpy( szaHeadLists, "0$ 0IN 0NNS 0NN 0JJ 0RB 0DT 0CD 0NCD 0QP 0JJR 0JJS 0" );
		else if( strcmp( pszCFGLeft, "RRC" ) == 0 )
			strcpy( szaHeadLists, "1VP 1NP 1ADVP 1ADJP 1PP 1" );
		else if( strcmp( pszCFGLeft, "S" ) == 0 )
			strcpy( szaHeadLists, "0TO 0IN 0VP 0S 0SBAR 0ADJP 0UCP 0NP 0" );
		else if( strcmp( pszCFGLeft, "SBAR" ) == 0 )
			strcpy( szaHeadLists, "0WHNP 0WHPP 0WHADVP 0WHADJP 0IN 0DT 0S 0SQ 0SINV 0SBAR 0FRAG 0" );
		else if( strcmp( pszCFGLeft, "SBARQ" ) == 0 )
			strcpy( szaHeadLists, "0SQ 0S 0SINV 0SBARQ 0FRAG 0" );
		else if( strcmp( pszCFGLeft, "SINV" ) == 0 )
			strcpy( szaHeadLists, "0VBZ 0VBD 0VBP 0VB 0MD 0VP 0S 0SINV 0ADJP 0NP 0" );
		else if( strcmp( pszCFGLeft, "SQ" ) == 0 )
			strcpy( szaHeadLists, "0VBZ 0VBD 0VBP 0VB 0MD 0VP 0SQ 0" );
		else if( strcmp( pszCFGLeft, "UCP" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "VP" ) == 0 )
			strcpy( szaHeadLists, "0TO 0VBD 0VBN 0MD 0VBZ 0VB 0VBG 0VBP 0VP 0ADJP 0NN 0NNS 0NP 0" );
		else if( strcmp( pszCFGLeft, "WHADJP" ) == 0 )
			strcpy( szaHeadLists, "0CC 0WRB 0JJ 0ADJP 0" );
		else if( strcmp( pszCFGLeft, "WHADVP" ) == 0 )
			strcpy( szaHeadLists, "1CC 1WRB 1" );
		else if( strcmp( pszCFGLeft, "WHNP" ) == 0 )
			strcpy( szaHeadLists, "0WDT 0WP 0WP$ 0WHADJP 0WHPP 0WHNP 0" );
		else if( strcmp( pszCFGLeft, "WHPP" ) == 0 )
			strcpy( szaHeadLists, "1IN 1TO FW 1" );
		else if( strcmp( pszCFGLeft, "NP" ) == 0 )
			strcpy( szaHeadLists, "0NN NNP NNS NNPS NX POS JJR 0NP 0$ ADJP PRN 0CD 0JJ JJS RB QP 0" );
		*/

		if( strcmp( pszCFGLeft, "ADJP" ) == 0 )
			strcpy( szaHeadLists, "0ADJP JJ 0AD NN CS 0" );
		else if( strcmp( pszCFGLeft, "ADVP" ) == 0 )
			strcpy( szaHeadLists, "0ADVP AD 0" );
		else if( strcmp( pszCFGLeft, "CLP" ) == 0 )
			strcpy( szaHeadLists, "0CLP M 0" );
		else if( strcmp( pszCFGLeft, "CP" ) == 0 )
			strcpy( szaHeadLists, "0DEC SP 1ADVP CS 0CP IP 0" );
		else if( strcmp( pszCFGLeft, "DNP" ) == 0 )
			strcpy( szaHeadLists, "0DNP DEG 0DEC 0" );
		else if( strcmp( pszCFGLeft, "DVP" ) == 0 )
			strcpy( szaHeadLists, "0DVP DEV 0" );
		else if( strcmp( pszCFGLeft, "DP" ) == 0 )
			strcpy( szaHeadLists, "1DP DT 1" );
		else if( strcmp( pszCFGLeft, "FRAG" ) == 0 )
			strcpy( szaHeadLists, "0VV NR NN 0" );
		else if( strcmp( pszCFGLeft, "INTJ" ) == 0 )
			strcpy( szaHeadLists, "0INTJ IJ 0" );
		else if( strcmp( pszCFGLeft, "LST" ) == 0 )
			strcpy( szaHeadLists, "1LST CD OD 1" );
		else if( strcmp( pszCFGLeft, "IP" ) == 0 )
			strcpy( szaHeadLists, "0IP VP 0VV 0" );
		else if( strcmp( pszCFGLeft, "LCP" ) == 0 )
			strcpy( szaHeadLists, "0LCP LC 0" );
		else if( strcmp( pszCFGLeft, "NP" ) == 0 )
			strcpy( szaHeadLists, "0NP NN NT NR QP 0" );
		else if( strcmp( pszCFGLeft, "PP" ) == 0 )
			strcpy( szaHeadLists, "1PP P 1" );
		else if( strcmp( pszCFGLeft, "PRN" ) == 0 )
			strcpy( szaHeadLists, "0 NP IP VP NT NR NN 0" );
		else if( strcmp( pszCFGLeft, "QP" ) == 0 )
			strcpy( szaHeadLists, "0QP CLP CD OD 0" );
		else if( strcmp( pszCFGLeft, "VP" ) == 0 )
			strcpy( szaHeadLists, "1VP VA VC VE VV BA LB VCD VSB VRD VNV VCP 1" );
		else if( strcmp( pszCFGLeft, "VCD" ) == 0 )
			strcpy( szaHeadLists, "0VCD VV VA VC VE 0" );
		if( strcmp( pszCFGLeft, "VRD" ) == 0 )
			strcpy( szaHeadLists, "0VRD VV VA VC VE 0" );
		else if( strcmp( pszCFGLeft, "VSB" ) == 0 )
			strcpy( szaHeadLists, "0VSB VV VA VC VE 0" );
		else if( strcmp( pszCFGLeft, "VCP" ) == 0 )
			strcpy( szaHeadLists, "0VCP VV VA VC VE 0" );
		else if( strcmp( pszCFGLeft, "VNV" ) == 0 )
			strcpy( szaHeadLists, "0VNV VV VA VC VE 0" );
		else if( strcmp( pszCFGLeft, "VPT" ) == 0 )
			strcpy( szaHeadLists, "0VNV VV VA VC VE 0" );
		else if( strcmp( pszCFGLeft, "UCP" ) == 0 )
			strcpy( szaHeadLists, "0" );
		else if( strcmp( pszCFGLeft, "WHNP" ) == 0 )
			strcpy( szaHeadLists, "0WHNP NP NN NT NR QP 0" );
		else if( strcmp( pszCFGLeft, "WHPP" ) == 0 )
			strcpy( szaHeadLists, "1WHPP PP P 1" );

		/*  //head rules for GENIA corpus
		if( strcmp( pszCFGLeft, "ADJP" ) == 0 )
			strcpy( szaHeadLists, "0NNS 0QP 0NN 0$ 0ADVP 0JJ 0VBN 0VBG 0ADJP 0JJR 0NP 0JJS 0DT 0FW 0RBR 0RBS 0SBAR 0RB 0" );
		else if( strcmp( pszCFGLeft, "ADVP" ) == 0 )
			strcpy( szaHeadLists, "1RB 1RBR 1RBS 1FW 1ADVP 1TO 1CD 1JJR 1JJ 1IN 1NP 1JJS 1NN 1" );
		else if( strcmp( pszCFGLeft, "CONJP" ) == 0 )
			strcpy( szaHeadLists, "1CC 1RB 1IN 1" );
		else if( strcmp( pszCFGLeft, "FRAG" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "INTJ" ) == 0 )
			strcpy( szaHeadLists, "0" );
		else if( strcmp( pszCFGLeft, "LST" ) == 0 )
			strcpy( szaHeadLists, "1LS 1: 1CLN 1" );
		else if( strcmp( pszCFGLeft, "NAC" ) == 0 )
			strcpy( szaHeadLists, "0NN 0NNS 0NNP 0NNPS 0NP 0NAC 0EX 0$ 0CD 0QP 0PRP 0VBG 0JJ 0JJS 0JJR 0ADJP 0FW 0" );
		else if( strcmp( pszCFGLeft, "PP" ) == 0 )
			strcpy( szaHeadLists, "1IN 1TO 1VBG 1VBN 1RP 1FW 1" );
		else if( strcmp( pszCFGLeft, "PRN" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "PRT" ) == 0 )
			strcpy( szaHeadLists, "1RP 1" );
		else if( strcmp( pszCFGLeft, "QP" ) == 0 )
			strcpy( szaHeadLists, "0$ 0IN 0NNS 0NN 0JJ 0RB 0DT 0CD 0NCD 0QP 0JJR 0JJS 0" );
		else if( strcmp( pszCFGLeft, "RRC" ) == 0 )
			strcpy( szaHeadLists, "1VP 1NP 1ADVP 1ADJP 1PP 1" );
		else if( strcmp( pszCFGLeft, "S" ) == 0 )
			strcpy( szaHeadLists, "0TO 0IN 0VP 0S 0SBAR 0ADJP 0UCP 0NP 0" );
		else if( strcmp( pszCFGLeft, "SBAR" ) == 0 )
			strcpy( szaHeadLists, "0WHNP 0WHPP 0WHADVP 0WHADJP 0IN 0DT 0S 0SQ 0SINV 0SBAR 0FRAG 0" );
		else if( strcmp( pszCFGLeft, "SBARQ" ) == 0 )
			strcpy( szaHeadLists, "0SQ 0S 0SINV 0SBARQ 0FRAG 0" );
		else if( strcmp( pszCFGLeft, "SINV" ) == 0 )
			strcpy( szaHeadLists, "0VBZ 0VBD 0VBP 0VB 0MD 0VP 0S 0SINV 0ADJP 0NP 0" );
		else if( strcmp( pszCFGLeft, "SQ" ) == 0 )
			strcpy( szaHeadLists, "0VBZ 0VBD 0VBP 0VB 0MD 0VP 0SQ 0" );
		else if( strcmp( pszCFGLeft, "UCP" ) == 0 )
			strcpy( szaHeadLists, "1" );
		else if( strcmp( pszCFGLeft, "VP" ) == 0 )
			strcpy( szaHeadLists, "0TO 0VBD 0VBN 0MD 0VBZ 0VB 0VBG 0VBP 0VP 0ADJP 0NN 0NNS 0NP 0" );
		else if( strcmp( pszCFGLeft, "WHADJP" ) == 0 )
			strcpy( szaHeadLists, "0CC 0WRB 0JJ 0ADJP 0" );
		else if( strcmp( pszCFGLeft, "WHADVP" ) == 0 )
			strcpy( szaHeadLists, "1CC 1WRB 1" );
		else if( strcmp( pszCFGLeft, "WHNP" ) == 0 )
			strcpy( szaHeadLists, "0WDT 0WP 0WP$ 0WHADJP 0WHPP 0WHNP 0" );
		else if( strcmp( pszCFGLeft, "WHPP" ) == 0 )
			strcpy( szaHeadLists, "1IN 1TO FW 1" );
		else if( strcmp( pszCFGLeft, "NP" ) == 0 )
			strcpy( szaHeadLists, "0NN NNP NNS NNPS NX POS JJR 0NP 0$ ADJP PRN 0CD 0JJ JJS RB QP 0" );
		*/

		return fnMyOwnHeadWordRule( szaHeadLists, vectRight );
	}

private:
	int fnMyOwnHeadWordRule( char *pszaHeadLists, CVectorStr vectRight ) {
		char szHeadList[ 201 ], *p;
		char szTerm[ 101 ];
		int J;

		p = pszaHeadLists;

		int iCountRight;

		iCountRight = vectRight.size( );

		szHeadList[ 0 ] = '\0';
		while( 1 ){
			szTerm[ 0 ] = '\0';
			sscanf( p, "%s", szTerm );
			if( strlen( szHeadList ) == 0 ){
				if( strcmp( szTerm, "0" ) == 0 ){
					return iCountRight - 1;
				}
				if( strcmp( szTerm, "1" ) == 0 ){
					return 0;
				}

				sprintf( szHeadList, "%c %s ", szTerm[ 0 ], szTerm + 1 );
				p = strstr( p, szTerm );
				p += strlen( szTerm );
			}
			else{
				if(   ( szTerm[ 0 ] == '0' )
					||( szTerm[ 0 ] == '1' ) ){
					if( szHeadList[ 0 ] == '0' ){
						for( J = iCountRight - 1; J >= 0; J -- ){
							sprintf( szTerm, " %s ", vectRight.at( J ).c_str( ) );
							if( strstr( szHeadList, szTerm ) != NULL )
								return J;
						}
					}
					else{
						for( J = 0; J < iCountRight; J ++ ){
							sprintf( szTerm, " %s ", vectRight.at( J ).c_str( ) );
							if(	strstr( szHeadList, szTerm ) != NULL )
								return J;
						}
					}

					szHeadList[ 0 ] = '\0';
				}
				else{
					strcat( szHeadList, szTerm );
					strcat( szHeadList, " " );

					p = strstr( p, szTerm );
					p += strlen( szTerm );
				}
			}
		}

		return 0;
	}

};

struct SParsedTree{
	SParsedTree( ) {
		m_ptRoot = NULL;

		m_pbConstraint = NULL;
	}
	~SParsedTree( ) {
		if (m_ptRoot != NULL)
			delete m_ptRoot;

		if (m_pbConstraint != NULL)
			delete [] m_pbConstraint;
	}
	static SParsedTree* fnConvertFromString(const char* pszStr) {
		SParsedTree* pTree = new SParsedTree();

		vector<string> vecSyn;
		fnReadSyntactic(pszStr, vecSyn);

		int iLeft = 1, iRight = 1; //# left/right parenthesis

		STreeItem *pcurrent;

		pTree->m_ptRoot = new STreeItem(vecSyn[1].c_str());

		pcurrent = pTree->m_ptRoot;

		for (size_t i = 2; i < vecSyn.size() - 1; i++) {
			if ( strcmp(vecSyn[i].c_str(), "(") == 0 )
					iLeft++;
			else if (strcmp(vecSyn[i].c_str(), ")") == 0 ) {
				iRight++;
				if (pcurrent == NULL) {
					//error
					fprintf(stderr, "ERROR in ConvertFromString\n");
					fprintf(stderr, "%s\n", pszStr);
					return NULL;
				}
				pcurrent = pcurrent->m_ptParent;
			} else {
				STreeItem *ptNewItem = new STreeItem(vecSyn[i].c_str());
				pcurrent->fnAppend( ptNewItem );
				pcurrent = ptNewItem;

				if (strcmp(vecSyn[i - 1].c_str(), "(" ) != 0
						&& strcmp(vecSyn[i - 1].c_str(), ")" ) != 0 ) {
					pTree->m_vecTerminals.push_back(ptNewItem);
					pcurrent = pcurrent->m_ptParent;
				}
			}
		}

		if ( iLeft != iRight ) {
			//error
			fprintf(stderr, "the left and right parentheses are not matched!");
			fprintf(stderr, "ERROR in ConvertFromString\n");
			fprintf(stderr, "%s\n", pszStr);
			return NULL;
		}

		for (size_t i = 0; i < pTree->m_vecTerminals.size(); i++ )
			pTree->m_vecTerminals[i]->m_iHeadWordIndex = i;

		return pTree;
	}

	int fnGetNumWord() {
		return m_vecTerminals.size();
	}

	inline int fnGetOff(int i, int j) {
		assert(i < j);

		int n = fnGetNumWord();
		return (((2 * n - 1) * i - i * i) / 2) + j - i - 1;
	}
	void fnInitializeConstraint(bool bFlattened) {
		if (m_pbConstraint != NULL)
			return;

		int n = fnGetNumWord();
		m_pbConstraint = new char[n * (n - 1) / 2];
		memset(m_pbConstraint, 0, n * (n - 1) / 2);
		int position;

		for (int i = 0; i < n; i++) {
			for (int j = i + 1; j < n; j++) {
				if ((bFlattened == false && fnIsDepFixFloatStructure(i, j) == true)
					|| (bFlattened == true && fnIsDepFixFloatStructureFlattened(i, j) == true)) {
					position = fnGetOff(i, j);
					assert(position < n * (n - 1) / 2);
					m_pbConstraint[position] = 1;
				}
			}
		}
	}

	char fnIsMeetConstraint(int i, int j, bool bFlattened) {
		if (m_pbConstraint == NULL)
			fnInitializeConstraint(bFlattened);
		int position = fnGetOff(i, j);
		return m_pbConstraint[position];
	}

	void fnSetSpanInfo() {
		int iNextNum = 0;
		fnSuffixTraverseSetSpanInfo(m_ptRoot, iNextNum);
	}

	void fnSetHeadWord() {
		SGetHeadWord *pGetHeadWord = new SGetHeadWord();
		fnSuffixTraverseSetHeadWord(m_ptRoot, pGetHeadWord);
		delete pGetHeadWord;
	}

private:
	void fnSuffixTraverseSetSpanInfo(STreeItem *ptItem, int& iNextNum) {
		int I;
		int iNumChildren = ptItem->fnGetChildrenNum();
		for ( I = 0; I < iNumChildren; I++ )
			fnSuffixTraverseSetSpanInfo(ptItem->m_vecChildren[ I ], iNextNum);

		if ( I == 0 )
		{
			ptItem->m_iBegin = iNextNum;
			ptItem->m_iEnd = iNextNum++;
		}
		else
		{
			ptItem->m_iBegin = ptItem->m_vecChildren[0]->m_iBegin;
			ptItem->m_iEnd = ptItem->m_vecChildren[I - 1]->m_iEnd;
		}
	}


	void fnSuffixTraverseSetHeadWord(STreeItem *ptItem, SGetHeadWord *pGetHeadWord) {
		int I, iHeadIndex;

		if ( ptItem->m_vecChildren.size() == 0 )
			return;

		for ( I = 0; I < ptItem->m_vecChildren.size(); I++ )
			fnSuffixTraverseSetHeadWord(ptItem->m_vecChildren[I], pGetHeadWord);

		vector<string> vecRight;


		if ( ptItem->m_vecChildren.size() == 1 )
			iHeadIndex = 0;
		else
		{
			for ( I = 0; I < ptItem->m_vecChildren.size(); I++ )
				vecRight.push_back( string( ptItem->m_vecChildren[ I ]->m_pszTerm ) );

			iHeadIndex = pGetHeadWord->fnGetHeadWord( ptItem->m_pszTerm, vecRight );
		}

		ptItem->m_iHeadIndex = iHeadIndex;

		ptItem->m_iHeadWordIndex = ptItem->m_vecChildren[ iHeadIndex ]->m_iHeadWordIndex;

		STreeItem *pTmpItem;

		if ( ptItem->fnIsPreTerminal( ) == true )
		{
		}
		else
		{
			//set dependency head
			for ( I = 0; I < ptItem->m_vecChildren.size(); I++ )
			{
				if ( I != ptItem->m_iHeadIndex )
				{
					pTmpItem = ptItem->m_vecChildren[ I ];
					while ( true )
					{
						pTmpItem->m_iDepHeadWordIndex = ptItem->m_iHeadWordIndex;

						if ( pTmpItem->m_vecChildren.size() > 0 )
							pTmpItem = pTmpItem->m_vecChildren[ pTmpItem->m_iHeadIndex ];
						else
							break;
					}
				}
			}
		}
	}
	bool fnIsDepFixFloatStructureFlattened(int iBegin, int iEnd) {
		SParsedTree *pTree = this;
		if ( pTree->m_ptRoot->m_iBegin == -1 )
			pTree->fnSetSpanInfo( );
		STreeItem *pItem;
		pItem = pTree->m_vecTerminals[iBegin]->m_ptParent;
		STreeItem *pCommonParent = NULL;
		while ( true ) {
			while (pItem->m_ptParent != NULL && pItem->m_ptParent->m_iBegin == pItem->m_iBegin && pItem->m_ptParent->m_iEnd <= iEnd)
				pItem = pItem->m_ptParent;

			if (pCommonParent == NULL)
				pCommonParent = pItem->m_ptParent;
			if (pCommonParent != NULL && pItem->m_ptParent != pCommonParent)
				return false;


			if ( pItem->m_iEnd < iEnd )
				pItem = pTree->m_vecTerminals[ pItem->m_iEnd + 1 ]->m_ptParent;
			else
				break;
		}

		return true;

	}
	bool fnIsDepFixFloatStructure(int iBegin, int iEnd) {
		SParsedTree *pTree = this;
		if ( pTree->m_ptRoot->m_iBegin == -1 )
			pTree->fnSetSpanInfo( );
		if ( pTree->m_ptRoot->m_iHeadIndex == -1 )
			pTree->fnSetHeadWord( );

		vector<STreeItem*> vecTreeItem;
		STreeItem *pItem;
		pItem = pTree->m_vecTerminals[iBegin]->m_ptParent;

		while ( true )
		{
			while (pItem->m_ptParent != NULL && pItem->m_ptParent->m_iBegin == pItem->m_iBegin && pItem->m_ptParent->m_iEnd <= iEnd)
				pItem = pItem->m_ptParent;

			vecTreeItem.push_back(pItem);

			if ( pItem->m_iEnd < iEnd )
				pItem = pTree->m_vecTerminals[ pItem->m_iEnd + 1 ]->m_ptParent;
			else
				break;
		}

		int iSize;
		iSize = vecTreeItem.size( );
		if ( iSize == 1 )
			return true; //fix structure, [iBegin, iEnd] corresponds to a node

		int I;
		int iHead = -1; //head among the items if it has, head item must be uncompleted note, and all other items must be completed
		for ( I = 0; I < iSize; I++ )
			if ( vecTreeItem[ I ]->m_ptParent->m_iHeadIndex == vecTreeItem[I]->m_iBrotherIndex ) {
				//it's a head node
				if ( iHead != -1 )
					return false; //already found a head
				iHead = I;
			}

		if ( iHead == -1 ) {
			//no head found, all are completed, then they must have the same dependent node
			for ( I = 1; I < iSize; I++ )
				if ( vecTreeItem[ I ]->m_iDepHeadWordIndex != vecTreeItem[ I - 1 ]->m_iDepHeadWordIndex )
					break;
			if ( I == iSize )
				return true;
			else
				return false;
		} else {
			//one head is found, all others must be dependent on it
			for ( I = 0; I < iSize; I++ ) {
				if ( I == iHead )
					continue;

				//all other items are completed, and dependent on iHead
				if ( vecTreeItem[ I ]->m_iDepHeadWordIndex != vecTreeItem[ iHead ]->m_iHeadWordIndex
						|| vecTreeItem[ I ]->m_ptParent->m_iHeadIndex == vecTreeItem[ I ]->m_iBrotherIndex )
					break;
			}
			if ( I == iSize )
				return true;
			else
				return false;
		}
	}
public:
	STreeItem *m_ptRoot;
	vector<STreeItem*>  m_vecTerminals; //the leaf nodes

	char* m_pbConstraint;
};

struct SParseReader {
	SParseReader(const char* pszParse_Fname) {
		m_fpIn = fopen( pszParse_Fname, "r" );
		assert(m_fpIn != NULL);
	}
	~SParseReader() {
		if (m_fpIn != NULL)
			fclose(m_fpIn);
	}

	SParsedTree* fnReadNextParseTree( ) {
		SParsedTree *pTree = NULL;
		char *pszLine = new char[100001];
		int iLen;

		while (fnReadNextSentence(pszLine, &iLen) == true ) {
			if (iLen == 0)
				continue;

			pTree = SParsedTree::fnConvertFromString(pszLine);
			break;
		}

		delete [] pszLine;
		return pTree;
	}
private:
	bool fnReadNextSentence(char *pszLine, int* piLength) {
		if (feof(m_fpIn) == true)
			return false;

		int iLen;

		pszLine[ 0 ] = '\0';

		fgets(pszLine, 10001, m_fpIn);
		iLen = strlen(pszLine);
		while (iLen > 0 && pszLine[iLen - 1] > 0 && pszLine[iLen -1] < 33) {
			pszLine[ iLen - 1 ] = '\0';
			iLen--;
		}

		if ( piLength != NULL )
			(*piLength) = iLen;

		return true;
	}
private:
	FILE *m_fpIn;

};

struct SExtract_SynCon
{
public:
	SExtract_SynCon(const char* pszParse_Fname, const char* pszOutput_Fname, bool bFlattened) {
		fnExtract_Syn_Constraitn(pszParse_Fname, pszOutput_Fname, bFlattened);
	}
	~SExtract_SynCon( ) {

	}

private:
	void fnExtract_Syn_Constraitn(const char* pszParse_Fname, const char* pszOutput_Fname, bool bFlattened) {
		gzFile fpOut = gzopen( pszOutput_Fname, "w" );
		assert( fpOut != NULL );

		SParseReader *pPReader = new SParseReader(pszParse_Fname);

		SParsedTree *pTree;
		int I, J;
		char *pszLine;
		pszLine = new char[ 100001 ];
		int iNum_Sent = 0;
		while ( ( pTree = pPReader->fnReadNextParseTree( ) ) != NULL ) {
			pTree->fnSetSpanInfo( );
			if (bFlattened == false) {
				pTree->fnSetHeadWord( );
			}

			pTree->fnInitializeConstraint(bFlattened);

			gzprintf( fpOut, "%d\n", pTree->m_vecTerminals.size() );
			for ( I = 0; I < pTree->m_vecTerminals.size(); I++ )
			{
				pszLine[ 0 ] = '\0';
				for ( J = I + 1; J < pTree->m_vecTerminals.size(); J++ )
					if ( pTree->fnIsMeetConstraint(I, J, bFlattened) == 1 )
						sprintf( pszLine + strlen( pszLine ), "%d ", J );
				if ( pszLine[ 0 ] != '\0' )
					pszLine[ strlen( pszLine ) - 1 ] = '\0';
				gzprintf( fpOut, "%s\n", pszLine );
			}

			delete pTree;

			iNum_Sent++;
			if (iNum_Sent % 100000 == 0)
				fprintf( stderr, "....Processing $Sentence: %d\n", iNum_Sent );
		}

		fprintf( stderr, "....Processing $Sentence: %d\n", iNum_Sent );

		gzclose( fpOut );
		delete [] pszLine;
		delete pPReader;
	}
};


void fnPrintUsage( ) {
	fprintf( stdout, "extract_syncon input_parsed_file output_file [1|0]\n" );
	fprintf( stdout, "               1 for flattened parse tree\n" );
	fprintf( stdout, "               0 for unflattened parse tree\n" );
}

int main( int argc, char* argv[] )
{
	if ( argc != 4 ) {
		fnPrintUsage( );
		return -1;
	}
	fprintf( stderr, "Extract syntactic constraints....\n" );
	fprintf( stderr, "Syntactic file: %s\n", argv[ 1 ] );
	clock_t ctStart, ctFinish;
	ctStart = clock( );
	SExtract_SynCon *pExtract = new SExtract_SynCon( argv[ 1 ], argv[ 2 ], strcmp(argv[3], "1") == 0? true : false );
	delete pExtract;
	ctFinish = clock( );
	fprintf( stderr, "Syntactic constraint file saved as: %s\n", argv[ 2 ] );
	fprintf( stderr, "Finished extracting syntactic constraints....\n" );
	fprintf( stderr, "Total time for extraction: %.2f seconds\n", (double)( ctFinish - ctStart ) / CLOCKS_PER_SEC );
	return 1;
}

