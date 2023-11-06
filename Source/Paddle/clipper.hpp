#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <list>
#include <ostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <vector>

namespace ClipperLib {

class PolyNode;

struct TEdge;
struct IntersectNode;
struct LocalMinimum;
struct OutPt;
struct OutRec;
struct Join;

typedef std::vector<PolyNode*> PolyNodes;
typedef std::vector<OutRec*> PolyOutList;
typedef std::vector<TEdge*> EdgeList;
typedef std::vector<Join*> JoinList;
typedef std::vector<IntersectNode*> IntersectList;

enum ClipType { ctIntersection, ctUnion, ctDifference, ctXor };
enum PolyType { ptSubject, ptClip };
enum JoinType { jtSquare, jtRound, jtMiter };
enum EndType { etClosedPolygon, etClosedLine, etOpenButt, etOpenSquare };
enum PolyFillType { pftEvenOdd, pftNonZero, pftPositive, pftNegative };

enum InitOptions {
    ioReverseSolution = 1,
    ioStrictlySimple = 2,
    ioPreserveCollinear = 4
};

enum EdgeSide {
    esLeft = 1,
    esRight = 2
};

struct IntPoint {
    long long X{};
    long long Y{};

    IntPoint(long long x = 0, long long y = 0) : X{ x }, Y{ y } {}

    friend inline bool operator==(const IntPoint& a, const IntPoint& b) {
        return a.X == b.X && a.Y == b.Y;
    }

    friend inline bool operator!=(const IntPoint& a, const IntPoint& b) {
        return a.X != b.X || a.Y != b.Y;
    }
};

struct DoublePoint {
    double X{};
    double Y{};

    DoublePoint(double x = 0.0, double y = 0.0) : X{ x }, Y{ y } {}
    DoublePoint(IntPoint ip) : X{ static_cast<double>(ip.X) }, Y{ static_cast<double>(ip.Y) } {}
};

struct IntRect {
    long long left{};
    long long top{};
    long long right{};
    long long bottom{};
};

class PolyNode {
public:

    unsigned int Index{}; // node index in Parent.Children
    JoinType m_jointype;
    EndType m_endtype;
    std::vector<IntPoint> Contour;
    PolyNodes Children;

    int ChildCount() const;
    void AddChild(PolyNode& child);

};

bool Orientation(const std::vector<IntPoint>& poly);
double Area(const std::vector<IntPoint>& poly);

class ClipperBase {
public:

    ClipperBase();

    virtual ~ClipperBase();

    virtual bool AddPath(const std::vector<IntPoint>& pg, PolyType PolyTyp, bool Closed);
    bool AddPaths(const std::vector<std::vector<IntPoint>>& ppg, PolyType PolyTyp, bool Closed);
    virtual void Clear();
    IntRect GetBounds();

protected:

    void DisposeLocalMinimaList();

    virtual void Reset();

    TEdge* ProcessBound(TEdge* E, bool IsClockwise);
    void InsertScanbeam(long long Y);
    bool PopScanbeam(long long& Y);
    bool LocalMinimaPending();
    bool PopLocalMinima(long long Y, const LocalMinimum*& locMin);
    OutRec* CreateOutRec();
    void DisposeAllOutRecs();
    void DisposeOutRec(PolyOutList::size_type index);
    void SwapPositionsInAEL(TEdge* edge1, TEdge* edge2);
    void DeleteFromAEL(TEdge* e);
    void UpdateEdgeIntoAEL(TEdge*& e);

    typedef std::vector<LocalMinimum> MinimaList;
    MinimaList::iterator m_CurrentLM;
    MinimaList m_MinimaList;

    bool m_UseFullRange;
    EdgeList m_edges;
    bool m_PreserveCollinear;
    bool m_HasOpenPaths;
    PolyOutList m_PolyOuts;
    TEdge* m_ActiveEdges;

    typedef std::priority_queue<long long> ScanbeamList;
    ScanbeamList m_Scanbeam;
};

class Clipper : public virtual ClipperBase {
public:

    Clipper(int initOptions = 0);

    bool Execute(ClipType clipType, std::vector<std::vector<IntPoint>>& solution, PolyFillType subjFillType, PolyFillType clipFillType);

    void ReverseSolution(bool value) {
        m_ReverseOutput = value;
    }

protected:

    virtual bool ExecuteInternal();

private:

    typedef std::list<long long> MaximaList;

    JoinList m_Joins;
    JoinList m_GhostJoins;
    IntersectList m_IntersectList;
    ClipType m_ClipType;
    MaximaList m_Maxima;
    TEdge* m_SortedEdges{};
    bool m_ExecuteLocked{};
    PolyFillType m_ClipFillType;
    PolyFillType m_SubjFillType;
    bool m_ReverseOutput{};
    bool m_UsingPolyTree{};
    bool m_StrictSimple{};

    void SetWindingCount(TEdge& edge);
    bool IsEvenOddFillType(const TEdge& edge) const;
    bool IsEvenOddAltFillType(const TEdge& edge) const;
    void InsertLocalMinimaIntoAEL(long long botY);
    void InsertEdgeIntoAEL(TEdge* edge, TEdge* startEdge);
    void AddEdgeToSEL(TEdge* edge);
    bool PopEdgeFromSEL(TEdge*& edge);
    void CopyAELToSEL();
    void DeleteFromSEL(TEdge* e);
    void SwapPositionsInSEL(TEdge* edge1, TEdge* edge2);
    bool IsContributing(const TEdge& edge) const;
    void DoMaxima(TEdge* e);
    void ProcessHorizontals();
    void ProcessHorizontal(TEdge* horzEdge);
    void AddLocalMaxPoly(TEdge* e1, TEdge* e2, const IntPoint& pt);
    OutPt* AddLocalMinPoly(TEdge* e1, TEdge* e2, const IntPoint& pt);
    OutRec* GetOutRec(int idx);
    void AppendPolygon(TEdge* e1, TEdge* e2);
    void IntersectEdges(TEdge* e1, TEdge* e2, IntPoint& pt);
    OutPt* AddOutPt(TEdge* e, const IntPoint& pt);
    OutPt* GetLastOutPt(TEdge* e);
    bool ProcessIntersections(long long topY);
    void BuildIntersectList(long long topY);
    void ProcessIntersectList();
    void ProcessEdgesAtTopOfScanbeam(long long topY);
    void BuildResult(std::vector<std::vector<IntPoint>>& polys);
    void SetHoleState(TEdge* e, OutRec* outrec);
    void DisposeIntersectNodes();
    bool FixupIntersectionOrder();
    void FixupOutPolygon(OutRec& outrec);
    void FixupOutPolyline(OutRec& outrec);
    void AddJoin(OutPt* op1, OutPt* op2, IntPoint offPt);
    void ClearJoins();
    void ClearGhostJoins();
    void AddGhostJoin(OutPt* op, IntPoint offPt);
    bool JoinPoints(Join* j, OutRec* outRec1, OutRec* outRec2);
    void JoinCommonEdges();
    void DoSimplePolygons();
    void FixupFirstLefts1(OutRec* OldOutRec, OutRec* NewOutRec);
    void FixupFirstLefts2(OutRec* InnerOutRec, OutRec* OuterOutRec);
    void FixupFirstLefts3(OutRec* OldOutRec, OutRec* NewOutRec);

};

class ClipperOffset {
public:

    double MiterLimit;
    double ArcTolerance;

    ClipperOffset(double miterLimit = 2.0, double roundPrecision = 0.25);

    ~ClipperOffset();

    void AddPath(const std::vector<IntPoint>& path, JoinType joinType, EndType endType);
    void Execute(std::vector<std::vector<IntPoint>>& solution, double delta);
    void Clear();

private:

    std::vector<std::vector<IntPoint>> m_destPolys;
    std::vector<IntPoint> m_srcPoly;
    std::vector<IntPoint> m_destPoly;
    std::vector<DoublePoint> m_normals;
    double m_delta{};
    double m_sinA{};
    double m_sin{};
    double m_cos{};
    double m_miterLim{};
    double m_StepsPerRad{};
    IntPoint m_lowest;
    PolyNode m_polyNodes;

    void FixOrientations();
    void DoOffset(double delta);
    void OffsetPoint(int j, int& k, JoinType jointype);
    void DoSquare(int j, int k);
    void DoMiter(int j, int k, double r);
    void DoRound(int j, int k);

};

class clipperException : public std::exception {
public:

    clipperException(const char* description) : description{ description } {}

    ~clipperException() noexcept override = default;

    const char* what() const noexcept override {
        return description.c_str();
    }

private:

    std::string description;

};

}
